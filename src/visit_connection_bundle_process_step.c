/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

/* included in main.c */

int requestInput_findCrlfOffset(struct requestInput *req){
	int offset = 0;
	if(!req) return -1;
	while(1){
		offset = chunkStream_findByteOffsetFrom(req->chunks, '\r', offset);
		if(-1 == offset) return -1;
		if('\n' == chunkStream_byteAtRelativeOffset(req->chunks, offset + 1))
			return offset + 2;
		offset++;
	}
}

int requestInput_printHeaders(struct requestInput *req){
	struct linked_list *node;
	struct linked_list *head;
	struct extent *key;
	struct extent *value;
	if(!req) return 1;
	if(!(req->headers)) return 1;
	node = req->headers->head;
	while(node){
		head = (struct linked_list*)(node->data);
		node = node->next;
		key = (struct extent*)(head->data);
		value = (struct extent*)(head->next->data);
		printf("key=<%s> value=<%s>\n", key->bytes, value->bytes);
	}
	return 0;
}

int requestInput_consumeHeader(struct requestInput *req){
	int colon = 0;
	int toCrlf = 0;
	struct extent key;
	struct extent value;
	struct linked_list *node;
	char *ptr = 0;
	if(!req) return 1;
	if(req->headersDone) return 0;
	if(chunkStream_reduceCursor(req->chunks)) return 1;
	if('\r' == chunkStream_byteAtRelativeOffset(req->chunks, 0)){
		if('\n' != chunkStream_byteAtRelativeOffset(req->chunks, 1))
			return 1;
		req->headersDone = 1;
		chunkStream_seekForward(req->chunks, 2);
		return 0;
	}
	colon = chunkStream_findByteOffsetFrom(req->chunks, ':', 0);
	if(-1 == colon) return 1;
	if(' ' != chunkStream_byteAtRelativeOffset(req->chunks, colon + 1))
		return 1;
	toCrlf = chunkStream_findByteOffsetFrom(req->chunks, '\r', colon + 2);
	if('\n' != chunkStream_byteAtRelativeOffset(req->chunks, colon + toCrlf + 3))
		return 1;
	if(chunkStream_takeBytes(req->chunks, colon, &key)) return 2;
	chunkStream_seekForward(req->chunks, 2);
	if(chunkStream_takeBytes(req->chunks, toCrlf, &value)) return 2;
	chunkStream_seekForward(req->chunks, 2);
	ptr = palloc(req->pool, 3 * sizeof(struct linked_list) + 2 * sizeof(struct extent));
	node = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	dequoid_append(req->headers, (struct linked_list*)ptr, node);
	node = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	node->next = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	node->data = ptr;
	ptr += sizeof(struct extent);
	node->next->data = ptr;
	ptr = 0;
	node->next->next = 0;
	memcpy(node->data, &key, sizeof(struct extent));
	memcpy(node->next->data, &value, sizeof(struct extent));
	node = 0;
	return 0;
}

int requestInput_consumeLine(struct requestInput *req){
	struct extent cursor;
	int offset = 0;
	if(!req) return 1;
	offset = requestInput_findCrlfOffset(req);
	if(-1 == offset) return 1;
	if(chunkStream_takeBytes(req->chunks, offset, &cursor)) return 2;
	return chunkStream_append(req->body, cursor.bytes, cursor.len);
}

int requestInput_consumeLastLine(struct requestInput *req){
	int len;
	struct linked_list *current;
	struct extent cursor;
	len = ((struct extent*)(req->chunks->cursor_chunk->data))->len;
	len -= req->chunks->cursor_chunk_offset;
	current = req->chunks->cursor_chunk->next;
	while(current){
		len += ((struct extent*)(current->data))->len;
		current = current->next;
	}
	if(!len) return 0;
	chunkStream_takeBytes(req->chunks, len, &cursor);
	return chunkStream_append(req->body, cursor.bytes, cursor.len);
}

int requestInput_consumeMethod(struct requestInput *req){
	int len;
	struct extent storage;
	if(!req) return 1;
	if(req->method) return 0;
	len = chunkStream_findByteOffsetFrom(req->chunks, ' ', 0);
	if(-1 == len) return 1;
	if(chunkStream_takeBytes(req->chunks, len, &storage)) return 2;
	req->method = (struct extent*)palloc(req->pool, sizeof(struct extent));
	req->method->bytes = storage.bytes;
	req->method->len = storage.len;
	chunkStream_seekForward(req->chunks, 1);
	return 0;
}

int requestInput_consumeRequestUrl(struct requestInput *req){
	int len;
	struct extent storage;
	if(!req) return 1;
	if(req->requestUrl) return 0;
	len = chunkStream_findByteOffsetFrom(req->chunks, ' ', 0);
	if(-1 == len) return 1;
	if(chunkStream_takeBytes(req->chunks, len, &storage)) return 2;
	req->requestUrl = (struct extent*)palloc(req->pool, sizeof(struct extent));
	req->requestUrl->bytes = storage.bytes;
	req->requestUrl->len = storage.len;
	chunkStream_seekForward(req->chunks, 1);
	return 0;
}

int requestInput_consumeHttpVersion(struct requestInput *req){
	int len;
	struct extent storage;
	int ver;
	if(!req) return 1;
	if(-1 != req->httpMajorVersion) return 0;
	len = chunkStream_findByteOffsetFrom(req->chunks, '/', 0);
	if(4 != len) return 1;
	if(chunkStream_takeBytes(req->chunks, 5, &storage)) return 2;
	if(strncmp(storage.bytes, "HTTP/", storage.len + 1)) return 3;
	len = chunkStream_findByteOffsetFrom(req->chunks, '.', 0);
	if(-1 == len) return 4;
	if(len >= requestInput_findCrlfOffset(req)) return 5;
	if(chunkStream_takeBytes(req->chunks, len, &storage)) return 6;
	ver = atoi(storage.bytes);

	chunkStream_seekForward(req->chunks, 1);
	len = requestInput_findCrlfOffset(req);
	if(len < 2) return 7;
	if(chunkStream_takeBytes(req->chunks, len - 2, &storage)) return 8;
	req->httpMinorVersion = atoi(storage.bytes);
	req->httpMajorVersion = ver;
	chunkStream_seekForward(req->chunks, 2);
	return 0;
}

int requestInput_processStep(struct requestInput *req){
	/*
		https://tools.ietf.org/html/rfc7230#section-3.1.1
	*/
	int status = 0;
	if(!req) return 0;
	if(!(req->method)){
		if(requestInput_consumeMethod(req)) return 2;
		status = 1;
	}
	if(!(req->requestUrl)){
		if(requestInput_consumeRequestUrl(req)) return status ? status : 2;
		status = 1;
	}
	if(-1 == req->httpMinorVersion){
		if(requestInput_consumeHttpVersion(req)) return status ? status : 2;
		status = 1;
	}
	return status;
}

int connection_bundle_consume_header(struct conn_bundle *conn){
	int result;
	if(!conn) return 1;
	conn->input.headersDone = conn->done_reading_headers;
	conn->input.chunks = conn->chunk_stream;
	conn->input.pool = conn->pool;
	conn->input.headers = conn->request_headers;
	result = requestInput_consumeHeader(&(conn->input));
	conn->done_reading_headers = conn->input.headersDone;
	return result;
}

int connection_bundle_consume_line(struct conn_bundle *conn){
	if(!conn) return 1;
	conn->input.chunks = conn->chunk_stream;
	conn->input.body = conn->body;
	return requestInput_consumeLine(&(conn->input));
}

int connection_bundle_consume_last_line(struct conn_bundle *conn){
	if(!conn) return 1;
	conn->input.chunks = conn->chunk_stream;
	conn->input.body = conn->body;
	return requestInput_consumeLastLine(&(conn->input));
}

int connection_bundle_can_respondp(struct conn_bundle *conn){
	if(!(conn->method)) return 0;
	if(!(conn->request_url)) return 0;
	if(-1 == conn->http_major_version) return 0;
	if(-1 == conn->http_minor_version) return 0;
	if(conn->done_writing) return 0;
	return 1;
}

int connection_bundle_print_body(struct conn_bundle *conn){
	struct linked_list *node;
	node = conn->body->chunk_list.head;
	while(node){
		printf("%p\t%s", node->data, ((struct extent*)(node->data))->bytes);
		if(!(node->next))
			if('\n' != ((struct extent*)(node->data))->bytes[((struct extent*)(node->data))->len - 1])
				printf("\n");
		node = node->next;
	}
	return 0;
}

int connection_bundle_free(struct conn_bundle *conn){
	if(conn->fd == -1) return 0;
	close(conn->fd);
	conn->fd = -1;

	if(conn->pool){

		connection_bundle_consume_last_line(conn);

		free_pool(conn->pool);
		conn->pool = 0;
	}
	memset(conn, 0, sizeof(struct conn_bundle));
	conn->done_writing = 1;
	conn->done_reading = 1;
	conn->http_major_version = -1;
	conn->http_minor_version = -1;
	return 0;
}

int connection_bundle_close_write(struct conn_bundle *conn){
	if(conn->done_writing) return 0;
	conn->done_writing = 1;
	if(!(conn->done_reading)) return 0;
	return connection_bundle_free(conn);
}

int connection_bundle_write_crlf(struct conn_bundle *conn){
	return 2 != write(conn->fd, "\r\n", 2);
}

int connection_bundle_write_extent(struct conn_bundle *conn, struct extent *str){
	ssize_t bytes;
	bytes = write(conn->fd, str->bytes, str->len);
	if(bytes < 0) return 1;
	if((size_t)bytes != str->len) return 2;
	return 0;
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

int connection_bundle_respond(struct conn_bundle *conn){
	struct staticGetResource *resource;
	struct extent reason;
	struct linked_list *match_node = 0;
	if(conn->done_writing) return 0;

	resource = 0;
	if(!first_matching(conn->server->staticResources, (visitor_t)(&match_resource_url), (struct linked_list*)(conn->request_url), &match_node))
		if(match_node)
			resource = match_node->data;
	if(!resource)
		return connection_bundle_respond_bad_request_target(conn);

	if(strncmp(conn->method->bytes, "GET", conn->method->len + 1))
		return connection_bundle_respond_bad_method(conn);
	point_extent_at_nice_string(&reason, "OK");
	return connection_bundle_send_response(conn, 200, &reason, resource->headers, resource->body);
}

int visit_connection_bundle_process_step(struct conn_bundle *conn, int *context, struct linked_list *node){
	int result;
	(void)node;
	if(!conn) return 0;
	conn->input.method = conn->method;
	conn->input.requestUrl = conn->request_url;
	conn->input.httpMinorVersion = conn->http_minor_version;
	conn->input.chunks = conn->chunk_stream;
	conn->input.pool = conn->pool;
	conn->input.httpMajorVersion = conn->http_major_version;
	result = requestInput_processStep(&(conn->input));
	conn->method = conn->input.method;
	conn->request_url = conn->input.requestUrl;
	conn->http_major_version = conn->input.httpMajorVersion;
	conn->http_minor_version = conn->input.httpMinorVersion;
	if(result){
		if(result == 1)
			*context = 1;
		return 0;
	}

	if(connection_bundle_can_respondp(conn)){
		*context = 1;
		connection_bundle_respond(conn);
	}
	while(!(conn->done_reading_headers))
		if(!connection_bundle_consume_header(conn))
			*context = 1;
		else return 0;
	while(!connection_bundle_consume_line(conn)) *context = 1;
	if(conn->done_reading){
		connection_bundle_consume_last_line(conn);
		*context = 1;
	}
	chunkStream_reduceCursor(conn->chunk_stream);
	return 0;
}
