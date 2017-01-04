/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "./server.h"
#include "./request.h"

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

struct linked_list *push_header_contiguous(char *buffer, char *key, char *value, struct linked_list *next){
	char *ptr;
	struct linked_list *top;
	struct linked_list *key_node;
	struct linked_list *value_node;
	struct extent *key_extent;
	struct extent *value_extent;
	ptr = buffer;
	top = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	key_node = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	value_node = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	key_extent = (struct extent*)ptr;
	ptr += sizeof(struct extent);
	value_extent = (struct extent*)ptr;
	return push_header_nice_strings(top, key_node, value_node, key_extent, key, value_extent, value, next);
}

int connection_bundle_respond_bad_method(struct conn_bundle *conn){
	char buffer[sizeof(struct linked_list) * 3 + sizeof(struct extent) * 2];
	struct extent reason;
	struct extent body;
	int status_code = 405;
	if(point_extent_at_nice_string(&reason, "METHOD NOT ALLOWED")) return 1;
	if(!push_header_contiguous(buffer, "Allow", "GET", 0)) return 1;
	if(point_extent_at_nice_string(&body, "Method Not Allowed\r\nOnly GET requests are accepted here.\r\n\r\n")) return 1;
	return connection_bundle_send_response(conn, status_code, &reason, (struct linked_list*)buffer, &body);
}
int connection_bundle_respond_bad_request_target(struct conn_bundle *conn){
	struct extent reason;
	struct extent body;
	if(point_extent_at_nice_string(&reason, "NOT FOUND")) return 1;
	if(point_extent_at_nice_string(&body, "Not Found.\r\n\r\n")) return 2;
	return connection_bundle_send_response(conn, 404, &reason, 0, &body);
}
int connection_bundle_respond_html_ok(struct conn_bundle *conn, struct linked_list *headers, struct extent *body){
	char buffer[sizeof(struct linked_list) * 3 + sizeof(struct extent) * 2];
	struct extent reason;
	int status_code = 200;
	if(point_extent_at_nice_string(&reason, "OK")) return 1;
	if(!push_header_contiguous(buffer, "Content-Type", "text/html", headers)) return 1;
	return connection_bundle_send_response(conn, status_code, &reason, (struct linked_list*)buffer, body);
}

int match_resource_url(struct staticGetResource *resource, struct extent *url, struct linked_list *node){
	(void)node;
	if(!resource) return 0;
	if(!url) return 0;
	if(!(resource->url)) return 0;
	if(resource->url->len != url->len) return 0;
	return !strncmp(resource->url->bytes, url->bytes, url->len + 1);
}

int staticGetResource_urlMatchesp(struct httpResource *resource, struct extent *url){
	if(!resource) return 0;
	if(!(resource->context)) return 0;
	if(!url) return 0;
	return match_resource_url(resource->context, url, 0);
}

int staticGetResource_respond(struct httpResource *resource, struct conn_bundle *connection){
	struct extent reason;
	struct staticGetResource *response;
	if(!resource) return 1;
	if(!connection) return 1;
	response = (struct staticGetResource*)(resource->context);
	if(!response) return 1;
	if(!(connection->input.method))
		return connection_bundle_respond_bad_method(connection);
	if(3 != connection->input.method->len)
		return connection_bundle_respond_bad_method(connection);
	if(!(connection->input.method->bytes))
		return connection_bundle_respond_bad_method(connection);
	if(strncmp(connection->input.method->bytes, "GET", connection->input.method->len + 1))
		return connection_bundle_respond_bad_method(connection);
	point_extent_at_nice_string(&reason, "OK");
	return connection_bundle_send_response(connection, 200, &reason, response->headers, response->body);
}

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

int visit_connection_bundle_process_step(struct conn_bundle *conn, int *context, struct linked_list *node){
	(void)node;
	if(connection_bundle_process_steppedp(conn)) *context = 1;
	conn = 0;
	context = 0;
	node = 0;
	return 0;
}

int httpServer_stepConnections(struct httpServer *server){
	int any = 0;
	if(traverse_linked_list(server->connections, (visitor_t)(&visit_connection_bundle_process_step), &any)) return 0;
	httpServer_removeEmptyConnections(server);
	return any;
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

struct contiguousHtmlResource{
	struct linked_list link_node;
	struct httpResource resource;
	struct staticGetResource staticResource;
	struct extent url;
	struct extent body;
	struct linked_list headerTop;
	struct linked_list keyNode;
	struct linked_list valNode;
	struct extent key;
	struct extent val;
};

int contiguousHtmlResource_init(struct contiguousHtmlResource *res, char *url, char *body){
	if(!res) return 2;
	if(!url) return 3;
	if(!body) return 3;
	if(point_extent_at_nice_string(&(res->url), url)) return 4;
	if(point_extent_at_nice_string(&(res->body), body)) return 4;
	res->staticResource.url = &(res->url);
	res->staticResource.body = &(res->body);
	res->staticResource.headers = push_header_nice_strings(&(res->headerTop), &(res->keyNode), &(res->valNode), &(res->key), "Content-Type", &(res->val), "text/html", 0);
	return !(res->staticResource.headers);
}

int main(int argument_count, char* *arguments_vector){
	struct httpServer server;
	struct mempool serverAllocations;
	struct linked_list *match_node;
	int ready_fd = -1;
	int status = 0;

	struct contiguousHtmlResource rootResourceStorage;

	if(2 != argument_count) return 1;
	if(httpServer_init(&server, &serverAllocations)) return 2;

	status = contiguousHtmlResource_init(
		&rootResourceStorage,
		"/",
		"<html>\r\n <head>\r\n  <title>Hello World!</title>\r\n </head>\r\n <body>\r\n  <h1>Hello, World!</h1>\r\n  <p>\r\n   This webserver is written in C.\r\n   I'm pretty proud of it!\r\n  </p>\r\n  <p>Coming soon: a resource for the following form to POST to</p>\r\n  <form method=\"POST\" action=\"formtest/\">\r\n   <input type=\"submit\" />\r\n  </form>\r\n </body>\r\n</html>\r\n\r\n"
	);
	if(status) return 3;
	status = httpServer_pushResource(&server, &(rootResourceStorage.link_node), &(rootResourceStorage.resource), &staticGetResource_urlMatchesp, &staticGetResource_respond, &(rootResourceStorage.staticResource));
	if(status) return 4;

	if(httpServer_listen(&server, arguments_vector[1], 32)){
		server.listeningSocket_fileDescriptor = -1;
		httpServer_close(&server);
		return 5;
	}

	while(1){
		ready_fd = httpServer_selectRead(&server);
		if(-1 != ready_fd){
			if(ready_fd == server.listeningSocket_fileDescriptor)
				httpServer_acceptNewConnection(&server);
			else{
				match_node = 0;
				if(!first_matching(server.connections, (visitor_t)(&match_by_sockfd), (struct linked_list *)(&ready_fd), &match_node))
					if(match_node)
						httpRequestHandler_readChunk(match_node->data);
			}
		}
		if(!httpServer_stepConnections(&server))
			if(ready_fd == -1)
				usleep(10);
	}

	if(httpServer_close(&server)){
		server.listeningSocket_fileDescriptor = -1;
		return 6;
	}
	return 0;
}
