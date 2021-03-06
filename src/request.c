/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "./request.h"
#include "./server.h"

int incomingHttpRequest_init(struct incomingHttpRequest *ptr, struct httpServer *server, int fd){
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

int fd_canReadp(int fd){
	struct timeval timeout;
	fd_set to_read;
	int status;
	if(-1 == fd) return 0;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	FD_ZERO(&to_read);
	FD_SET(fd, &to_read);
	status = select(fd + 1, &to_read, 0, 0, &timeout);
	if(-1 == status) return 0;
	if(!status) return 0;
	if(!FD_ISSET(fd, &to_read)) return 0;
	FD_ZERO(&to_read);
	return 1;
}

int match_incomingHttpRequest_bySocketFileDescriptor(struct incomingHttpRequest *data, int *target, struct linked_list *node){
	(void)node;
	node = 0;
	if(!data) return 0;
	if(!target) return 0;
	return data->fd == *target;
}

int incomingHttpRequest_free(struct incomingHttpRequest *req){
	if(!req) return 1;
	if(req->fd == -1) return 0;
	close(req->fd);
	req->fd = -1;

	if(req->allocations.allocs){
		requestInput_consumeLastLine(&(req->input));

		free_pool(&(req->allocations));
		memset(&(req->allocations), 0, sizeof(struct mempool));
	}
	if(req->node)
		req->node->data = 0;
	memset(req, 0, sizeof(struct incomingHttpRequest));
	req->done_writing = 1;
	req->input.done = 1;
	req->input.httpMajorVersion = -1;
	req->input.httpMinorVersion = -1;
	free(req);
	return 0;
}
/* TODO: rename internal variables from conn to req */
int incomingHttpRequest_readChunk(struct incomingHttpRequest *conn){
	char buf[CHUNK_SIZE + 1];
	size_t len;

	if(!conn) return 2;

	buf[CHUNK_SIZE] = 0;
	len = read(conn->fd, buf, CHUNK_SIZE);
	requestInput_readChunk(&(conn->input), buf, len);

	if(!(conn->input.done)) return 0;
	if(conn->done_writing)
		return incomingHttpRequest_free(conn);
	return 0;
}

int incomingHttpRequest_writeExtent(struct incomingHttpRequest *conn, struct extent *str){
	ssize_t bytes;
	if(!(str->bytes)) return 0;
	bytes = write(conn->fd, str->bytes, str->len);
	if(bytes < 0) return 1;
	if((size_t)bytes != str->len) return 2;
	return 0;
}
int incomingHttpRequest_write_crlf(struct incomingHttpRequest *conn){
	return 2 != write(conn->fd, "\r\n", 2);
}

int incomingHttpRequest_write_statusLine(struct incomingHttpRequest *conn, int status_code, struct extent *reason){
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
	if(incomingHttpRequest_writeExtent(conn, reason)) return 1;
	return incomingHttpRequest_write_crlf(conn);
}

int incomingHttpRequest_write_header(struct incomingHttpRequest *conn, struct extent *key, struct extent *value){
	ssize_t bytes;
	if(incomingHttpRequest_writeExtent(conn, key)) return 1;
	bytes = write(conn->fd, ": ", 2);
	if(bytes < 0) return 1;
	if(2 != bytes) return 2;
	if(incomingHttpRequest_writeExtent(conn, value)) return 1;
	return incomingHttpRequest_write_crlf(conn);
}

int incomingHttpRequest_closeWrite(struct incomingHttpRequest *conn){
	if(conn->done_writing) return 0;
	conn->done_writing = 1;
	if(!(conn->input.done)) return 0;
	return incomingHttpRequest_free(conn);
}


int visit_header_write(struct linked_list *header, struct incomingHttpRequest *context, struct linked_list *node){
	(void)node;
	if(!header) return 1;
	if(!(header->data)) return 1;
	if(!(header->next)) return 1;
	if(!(header->next->data)) return 1;
	if(!context) return 1;
	return incomingHttpRequest_write_header(context, (struct extent*)(header->data), (struct extent*)(header->next->data));
}
int incomingHttpRequest_sendResponseHeaders(struct incomingHttpRequest *conn, int status_code, struct extent *reason, struct linked_list *headers){
	struct extent connection;
	struct extent close;
	if(point_extent_at_nice_string(&connection, "Connection")) return 1;
	if(point_extent_at_nice_string(&close, "close")) return 1;

	if(incomingHttpRequest_write_statusLine(conn, status_code, reason)) return 1;
	if(traverse_linked_list(headers, (visitor_t)(&visit_header_write), conn)) return 2;
	if(incomingHttpRequest_write_header(conn, &connection, &close)) return 3;
	if(incomingHttpRequest_write_crlf(conn)) return 4;
	return 0;
}

int incomingHttpRequest_beginChunkedResponse(struct incomingHttpRequest *req, int status_code, struct extent *reason, struct linked_list *headers){
	struct linked_list extraHead_node;
	struct linked_list extraHead_key;
	struct linked_list extraHead_val;
	struct extent key;
	struct extent val;
	if(!req) return 1;
	if(!reason) return 1;
	if(point_extent_at_nice_string(&key, "Transfer-Encoding")) return 2;
	if(point_extent_at_nice_string(&val, "chunked")) return 2;
	extraHead_node.data = &extraHead_key;
	extraHead_node.next = headers;
	extraHead_key.data = &key;
	extraHead_key.next = &extraHead_val;
	extraHead_val.data = &val;
	extraHead_val.next = 0;
	return incomingHttpRequest_sendResponseHeaders(req, status_code, reason, &extraHead_node);
}
int incomingHttpRequest_beginChunkedHtmlOk(struct incomingHttpRequest *req, struct linked_list *headers){
	struct extent reason;
	struct linked_list extraHead_node;
	struct linked_list extraHead_key;
	struct linked_list extraHead_val;
	struct extent key;
	struct extent val;
	if(!req) return 1;
	if(point_extent_at_nice_string(&reason, "OK")) return 2;
	if(point_extent_at_nice_string(&key, "Content-Type")) return 2;
	if(point_extent_at_nice_string(&val, "text/html")) return 2;
	extraHead_node.data = &extraHead_key;
	extraHead_node.next = headers;
	extraHead_key.data = &key;
	extraHead_key.next = &extraHead_val;
	extraHead_val.data = &val;
	extraHead_val.next = 0;
	return incomingHttpRequest_beginChunkedResponse(req, 200, &reason, &extraHead_node);
	return 1;
}

int incomingHttpRequest_write_chunk(struct incomingHttpRequest *req, char* bytes, size_t len){
	struct extent chunk;
	char chunk_length_str[256];
	if(!len) return 0;
	if(!req) return 1;
	if(!bytes) return 1;
	snprintf(chunk_length_str, 255, "%X", (int)len);
	chunk_length_str[255] = 0;
	if(point_extent_at_nice_string(&chunk, chunk_length_str)) return 2;
	if(incomingHttpRequest_writeExtent(req, &chunk)) return 3;
	if(incomingHttpRequest_write_crlf(req)) return 4;
	chunk.bytes = bytes;
	chunk.len = len;
	if(incomingHttpRequest_writeExtent(req, &chunk)) return 5;
	if(incomingHttpRequest_write_crlf(req)) return 6;
	return 0;
}
int incomingHttpRequest_writeChunk_niceString(struct incomingHttpRequest *req, char* str){
	return incomingHttpRequest_write_chunk(req, str, strlen(str));
}
int incomingHttpRequest_writelnChunk_niceString(struct incomingHttpRequest *req, char* str){
	int result = 0;
	if(!req) return 3;
	if(!str) return 4;
	if(*str)
		result += !!incomingHttpRequest_writeChunk_niceString(req, str);
	return result + !!incomingHttpRequest_writeChunk_niceString(req, "\r\n");
}
char* escapeHtmlByte(char b){
	if('<' == b) return "&lt;";
	if('>' == b) return "&gt;";
	if('"' == b) return "&quot;";
	if('\'' == b) return "&apos;";
	if('&' == b) return "&amp;";
	return 0;
}
int incomingHttpRequest_writeChunk_htmlSafeExtent(struct incomingHttpRequest *req, struct extent *str){
	struct extent chunk;
	char *cursor;
	size_t remaining;
	char *replacement;
	int status = 0;
	if(!req) return 1;
	if(!str) return 1;
	if(!(str->len)) return 0;
	if(!(str->bytes)) return 1;
	cursor = str->bytes;
	remaining = str->len;
	chunk.bytes = cursor;
	chunk.len = 0;
	while(remaining){
		replacement = escapeHtmlByte(chunk.bytes[chunk.len]);
		if(replacement){
			status += !!incomingHttpRequest_write_chunk(req, chunk.bytes, chunk.len);
			status += !!incomingHttpRequest_writeChunk_niceString(req, replacement);
			chunk.bytes += chunk.len + 1;
			chunk.len = 0;
		}
		else
			chunk.len++;
		remaining--;
	}
	status += !!incomingHttpRequest_write_chunk(req, chunk.bytes, chunk.len);
	return status;
}

int incomingHttpRequest_sendLastChunk(struct incomingHttpRequest *req, struct linked_list *trailers){
	struct extent lastChunk;
	if(!req) return 2;
	lastChunk.bytes = "0";
	lastChunk.len = 1;
	if(incomingHttpRequest_writeExtent(req, &lastChunk)) return 3;
	if(incomingHttpRequest_write_crlf(req)) return 4;
	if(traverse_linked_list(trailers, (visitor_t)(&visit_header_write), req)) return 5;
	if(incomingHttpRequest_write_crlf(req)) return 6;
	return incomingHttpRequest_closeWrite(req);
}


int incomingHttpRequest_beginChunkedHtmlBody(struct incomingHttpRequest *req, struct linked_list *headers, char *title, size_t titleLen){
	int status = 0;
	if(!req) return 1;
	if(titleLen)
		if(!title)
			return 2;
	if(incomingHttpRequest_beginChunkedHtmlOk(req, headers)) return 3;
	status += !!incomingHttpRequest_writelnChunk_niceString(req, "<html>");
	status += !!incomingHttpRequest_writelnChunk_niceString(req, " <head>");
	status += !!incomingHttpRequest_writeChunk_niceString(req, "  <title>");
	status += !!incomingHttpRequest_write_chunk(req, title, titleLen);
	status += !!incomingHttpRequest_writelnChunk_niceString(req, "</title>");
	status += !!incomingHttpRequest_writelnChunk_niceString(req, " </head>");
	status += !!incomingHttpRequest_writelnChunk_niceString(req, " <body>");
	if(status) return 4;
	return 0;
}
int incomingHttpRequest_endChunkedHtmlBody(struct incomingHttpRequest *req, struct linked_list *trailers){
	int status = 0;
	status += !!incomingHttpRequest_writelnChunk_niceString(req, " </body>");
	status *= 2;
	status += !!incomingHttpRequest_writelnChunk_niceString(req, "</html>");
	status *= 2;
	status += incomingHttpRequest_sendLastChunk(req, trailers);
	return status;
}


int incomingHttpRequest_sendResponse(struct incomingHttpRequest *conn, int status_code, struct extent *reason, struct linked_list *headers, struct extent *body){
	struct extent content_length_key;
	struct extent content_length_value;
	struct linked_list extraHeader_head;
	struct linked_list extraHeader_key;
	struct linked_list extraHeader_value;
	char content_length_str[256];
	if(point_extent_at_nice_string(&content_length_key, "Content-Length")) return 1;
	snprintf(content_length_str, 255, "%d", (int)(body->len));
	content_length_str[255] = 0;
	if(point_extent_at_nice_string(&content_length_value, content_length_str)) return 1;
	extraHeader_head.data = &extraHeader_key;
	extraHeader_head.next = headers;
	extraHeader_key.data = &content_length_key;
	extraHeader_key.next = &extraHeader_value;
	extraHeader_value.data = &content_length_value;
	extraHeader_value.next = 0;
	if(incomingHttpRequest_sendResponseHeaders(conn, status_code, reason, &extraHeader_head)) return 1;
	if(incomingHttpRequest_writeExtent(conn, body)) return 5;
	return incomingHttpRequest_closeWrite(conn);
}


int incomingHttpRequest_respond_notFound(struct incomingHttpRequest *conn){
	struct extent reason;
	struct extent body;
	if(point_extent_at_nice_string(&reason, "NOT FOUND")) return 1;
	if(point_extent_at_nice_string(&body, "Not Found.\r\n\r\n")) return 2;
	return incomingHttpRequest_sendResponse(conn, 404, &reason, 0, &body);
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
int incomingHttpRequest_respond_badMethod(struct incomingHttpRequest *conn){
	char buffer[sizeof(struct linked_list) * 3 + sizeof(struct extent) * 2];
	struct extent reason;
	struct extent body;
	int status_code = 405;
	if(point_extent_at_nice_string(&reason, "METHOD NOT ALLOWED")) return 1;
	if(!push_header_contiguous(buffer, "Allow", "GET", 0)) return 1;
	if(point_extent_at_nice_string(&body, "Method Not Allowed\r\nOnly GET requests are accepted here.\r\n\r\n")) return 1;
	return incomingHttpRequest_sendResponse(conn, status_code, &reason, (struct linked_list*)buffer, &body);
}
int incomingHttpRequest_respond_htmlOk(struct incomingHttpRequest *conn, struct linked_list *headers, struct extent *body){
	char buffer[sizeof(struct linked_list) * 3 + sizeof(struct extent) * 2];
	struct extent reason;
	int status_code = 200;
	if(point_extent_at_nice_string(&reason, "OK")) return 1;
	if(!push_header_contiguous(buffer, "Content-Type", "text/html", headers)) return 1;
	return incomingHttpRequest_sendResponse(conn, status_code, &reason, (struct linked_list*)buffer, body);
}


/* TODO: consider moving this method */
int httpResource_respond(struct httpResource *resource, struct incomingHttpRequest *connection){
	if(!connection) return 1;
	if(!resource) return 1;
	if(!(resource->respond)) return 1;
	return (*(resource->respond))(resource, connection);
}

int incomingHttpRequest_processSteppedp(struct incomingHttpRequest *conn){
	int status = 0;
	struct linked_list *old_node;
	struct httpResource *resource;
	if(!conn) return 0;
	status = 0;
	if(!(conn->input.done))
		if(fd_canReadp(conn->fd)){
			status = 1;
			old_node = conn->node;
			incomingHttpRequest_readChunk(conn);
			if(!old_node) return 1;
			if(!(old_node->data)) return 0;
		}
	if(1 == requestInput_processStep(&(conn->input))) return 1;
	if(!(conn->input.requestUrl)) return status;
	if(conn->done_writing) return status;
	resource = httpServer_locateResource(conn->server, conn->input.requestUrl);
	if(resource){
		if(resource->canRespondp)
			if(!((*(resource->canRespondp))(resource, conn))) return status;
		httpResource_respond(resource, conn);
		return 1;
	}
	if(!(conn->input.method)) return status;
	if(-1 == conn->input.httpMajorVersion) return status;
	if(-1 == conn->input.httpMinorVersion) return status;
	if(resource)
		httpResource_respond(resource, conn);
	else
		incomingHttpRequest_respond_notFound(conn);
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

int extent_url_equalsp(struct extent *target, struct extent *url){
	int targetLen = 0;
	int urlLen = 0;
	if(!target) return 0;
	if(!url) return 0;

	targetLen = target->len;
	if(!targetLen){
		target = 0;
		if(!url->len) return 1;
		if(!url->bytes) return 0;
		if(!(url->bytes[0])) return 1;
		if('/' != url->bytes[0]) return 0;
		if(url->bytes[1]) return 0;
		return 1;
	}
	if(!(target->bytes)) return 0;
	if('/' == target->bytes[targetLen - 1]) targetLen--;

	urlLen = url->len;
	if(!urlLen) return !targetLen;
	if(!(url->bytes)) return 0;
	if('/' == url->bytes[urlLen - 1]) urlLen--;

	if(targetLen != urlLen) return 0;
	return !strncmp(target->bytes, url->bytes, urlLen);
}
