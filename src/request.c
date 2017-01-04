/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "./request.h"
#include "./server.h"

/* TODO: rename move this method as appropriate */
int init_connection(struct conn_bundle *ptr, struct httpServer *server, int fd){
	memset(&(ptr->allocations), 0, sizeof(struct mempool));
	init_pool(&(ptr->allocations));
	ptr->fd = fd;
	fd = 0;
	ptr->done_writing = 0;
	ptr->server = server;
	server = 0;
	requestInput_init(&(ptr->input), &(ptr->allocations));
	ptr->input.fd = ptr->fd;
	ptr->node = 0;
	ptr = 0;
	return 0;
}

int incomingHttpRequest_selectRead(struct conn_bundle *req){
	struct timeval timeout;
	fd_set to_read;
	int status;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	FD_ZERO(&to_read);
	if(!req) return -1;
	if(req->input.done) return -1;
	if(-1 == req->fd) return -1;
	FD_SET(req->fd, &to_read);
	status = select(req->fd + 1, &to_read, 0, 0, &timeout);
	if(-1 == status) return -1;
	if(!status) return -1;
	if(!FD_ISSET(req->fd, &to_read)) return -1;
	FD_ZERO(&to_read);
	return req->fd;
}

int match_by_sockfd(struct conn_bundle *data, int *target, struct linked_list *node){
	(void)node;
	node = 0;
	if(!data) return 0;
	if(!target) return 0;
	return data->fd == *target;
}

/* TODO: extract a method on requestInput */
int httpRequestHandler_readChunk(struct conn_bundle *conn){
	char buf[CHUNK_SIZE + 1];
	size_t len;

	if(!conn) return 2;

	buf[CHUNK_SIZE] = 0;
	len = read(conn->fd, buf, CHUNK_SIZE);
	buf[len] = 0;
	chunkStream_append(conn->input.chunks, buf, len);

	if(len) return 0;
	conn->input.done = 1;
	if(conn->done_writing)
		return connection_bundle_free(conn);
	return 0;
}


int connection_bundle_can_respondp(struct conn_bundle *conn){
	if(!(conn->input.method)) return 0;
	if(!(conn->input.requestUrl)) return 0;
	if(-1 == conn->input.httpMajorVersion) return 0;
	if(-1 == conn->input.httpMinorVersion) return 0;
	if(conn->done_writing) return 0;
	return 1;
}


int connection_bundle_write_extent(struct conn_bundle *conn, struct extent *str){
	ssize_t bytes;
	bytes = write(conn->fd, str->bytes, str->len);
	if(bytes < 0) return 1;
	if((size_t)bytes != str->len) return 2;
	return 0;
}
int connection_bundle_write_crlf(struct conn_bundle *conn){
	return 2 != write(conn->fd, "\r\n", 2);
}

int connection_bundle_write_status_line(struct conn_bundle *conn, int status_code, struct extent *reason){
	char status_code_str[3];
	ssize_t bytes;
	if(status_code < 0) return 1;
	if(status_code > 999) return 1;
	status_code_str[0] = status_code / 100 + '1' - 1;
	status_code %= 100;
	status_code_str[1] = status_code / 10 + '1' - 1;
	status_code %= 10;
	status_code_str[2] = status_code + '1' - 1;
	bytes = write(conn->fd, "HTTP/1.1 ", 9);
	if(9 != bytes) return 1;
	bytes = write(conn->fd, status_code_str, 3);
	if(3 != bytes) return 1;
	bytes = write(conn->fd, " ", 1);
	if(1 != bytes) return 1;
	if(connection_bundle_write_extent(conn, reason)) return 1;
	return connection_bundle_write_crlf(conn);
}

int connection_bundle_write_header(struct conn_bundle *conn, struct extent *key, struct extent *value){
	ssize_t bytes;
	if(connection_bundle_write_extent(conn, key)) return 1;
	bytes = write(conn->fd, ": ", 2);
	if(bytes < 0) return 1;
	if(2 != bytes) return 2;
	if(connection_bundle_write_extent(conn, value)) return 1;
	return connection_bundle_write_crlf(conn);
}

int connection_bundle_free(struct conn_bundle *conn){
	if(!conn) return 1;
	if(conn->fd == -1) return 0;
	close(conn->fd);
	conn->fd = -1;

	if(conn->allocations.allocs){

		requestInput_consumeLastLine(&(conn->input));

		free_pool(&(conn->allocations));
		memset(&(conn->allocations), 0, sizeof(struct mempool));
	}
	if(conn->node) conn->node->data = 0;
	memset(conn, 0, sizeof(struct conn_bundle));
	conn->done_writing = 1;
	conn->input.done = 1;
	conn->input.httpMajorVersion = -1;
	conn->input.httpMinorVersion = -1;
	free(conn);
	return 0;
}
int connection_bundle_close_write(struct conn_bundle *conn){
	if(conn->done_writing) return 0;
	conn->done_writing = 1;
	if(!(conn->input.done)) return 0;
	return connection_bundle_free(conn);
}


int visit_header_write(struct linked_list *header, struct conn_bundle *context, struct linked_list *node){
	(void)node;
	if(!header) return 1;
	if(!(header->data)) return 1;
	if(!(header->next)) return 1;
	if(!(header->next->data)) return 1;
	if(!context) return 1;
	return connection_bundle_write_header(context, (struct extent*)(header->data), (struct extent*)(header->next->data));
}
int connection_bundle_send_response(struct conn_bundle *conn, int status_code, struct extent *reason, struct linked_list *headers, struct extent *body){
	struct extent connection;
	struct extent close;
	struct extent content_length_key;
	struct extent content_length_value;
	char content_length_str[256];
	if(point_extent_at_nice_string(&connection, "Connection")) return 1;
	if(point_extent_at_nice_string(&close, "close")) return 1;
	if(point_extent_at_nice_string(&content_length_key, "Content-Length")) return 1;
	snprintf(content_length_str, 255, "%d", (int)(body->len));
	content_length_str[255] = 0;
	if(point_extent_at_nice_string(&content_length_value, content_length_str)) return 1;
	if(connection_bundle_write_status_line(conn, status_code, reason)) return 1;
	if(traverse_linked_list(headers, (visitor_t)(&visit_header_write), conn)) return 2;
	if(connection_bundle_write_header(conn, &content_length_key, &content_length_value)) return 3;
	if(connection_bundle_write_header(conn, &connection, &close)) return 3;
	if(connection_bundle_write_crlf(conn)) return 4;
	if(connection_bundle_write_extent(conn, body)) return 5;
	return connection_bundle_close_write(conn);
}


int connection_bundle_respond_bad_request_target(struct conn_bundle *conn){
	struct extent reason;
	struct extent body;
	if(point_extent_at_nice_string(&reason, "NOT FOUND")) return 1;
	if(point_extent_at_nice_string(&body, "Not Found.\r\n\r\n")) return 2;
	return connection_bundle_send_response(conn, 404, &reason, 0, &body);
}



/* TODO: consider moving this method */
int httpResource_respond(struct httpResource *resource, struct conn_bundle *connection){
	if(!connection) return 1;
	if(!resource) return 1;
	if(!(resource->respond)) return 1;
	return (*(resource->respond))(resource, connection);
}


int connection_bundle_respond(struct conn_bundle *conn){
	struct httpResource *resource;
	struct linked_list *match_node = 0;
	if(conn->done_writing) return 0;

	resource = 0;
	if(!first_matching(conn->server->resources, (visitor_t)(&match_httpResource_url), (struct linked_list*)(conn->input.requestUrl), &match_node))
		if(match_node)
			resource = match_node->data;
	if(!resource)
		return connection_bundle_respond_bad_request_target(conn);
	return httpResource_respond(resource, conn);
}

int connection_bundle_process_steppedp(struct conn_bundle *conn){
	if(!conn) return 0;
	if(1 == requestInput_processStep(&(conn->input))) return 1;
	if(!connection_bundle_can_respondp(conn)) return 0;
	connection_bundle_respond(conn);
	return 1;
}


struct linked_list *push_header_nice_strings(struct linked_list *top, struct linked_list *key_node, struct linked_list *value_node, struct extent *key_extent, char *key, struct extent *value_extent, char *value, struct linked_list *next){
	top->data = key_node;
	top->next = next;
	key_node->data = key_extent;
	key_node->next = value_node;
	value_node->data = value_extent;
	value_node->next = 0;
	if(point_extent_at_nice_string(key_extent, key)) return 0;
	if(point_extent_at_nice_string(value_extent, value)) return 0;
	return top;
}
