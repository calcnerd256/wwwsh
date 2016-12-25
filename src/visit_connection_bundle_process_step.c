/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

/* included in main.c */

int connection_bundle_overstep_cursorp(struct conn_bundle *conn){
	if(!conn->cursor_chunk) return 1;
	return ((struct extent*)(conn->cursor_chunk->data))->len <= conn->cursor_chunk_offset;
}

int connection_bundle_reduce_cursor(struct conn_bundle *conn){
	if(!conn->cursor_chunk) conn->cursor_chunk = conn->chunks;
	if(!conn->cursor_chunk) return 1;
	while(connection_bundle_overstep_cursorp(conn)){
		if(!(conn->cursor_chunk->next)) return 2;
		conn->cursor_chunk_offset -= ((struct extent*)(conn->cursor_chunk->data))->len;
		conn->cursor_chunk = conn->cursor_chunk->next;
	}
	return 0;
}

int connection_bundle_advance_cursor(struct conn_bundle *conn, size_t delta){
	if(connection_bundle_reduce_cursor(conn)) return 1;
	conn->cursor_chunk_offset += delta;
	return connection_bundle_reduce_cursor(conn);
}

int connection_bundle_take_bytes(struct conn_bundle *conn, size_t len, struct extent *result){
	size_t l;
	char *ptr = 0;
	result->bytes = palloc(conn->pool, len + 1);
	memset(result->bytes, 0, len + 1);
	result->len = len;
	ptr = result->bytes;
	if(!(conn->cursor_chunk)) conn->cursor_chunk = conn->chunks;
	while(len){
		if(!(conn->cursor_chunk)) return 1;
		l = ((struct extent*)(conn->cursor_chunk->data))->len;
		if(conn->cursor_chunk_offset >= l){
			conn->cursor_chunk_offset -= l;
			conn->cursor_chunk = conn->cursor_chunk->next;
			continue;
		}
		if(len + conn->cursor_chunk_offset > l){
			l -= conn->cursor_chunk_offset;
			memcpy(ptr, ((struct extent*)(conn->cursor_chunk->data))->bytes + conn->cursor_chunk_offset, l);
			ptr += l;
			connection_bundle_advance_cursor(conn, l);
			len -= l;
		}
		else{
			memcpy(ptr, ((struct extent*)(conn->cursor_chunk->data))->bytes + conn->cursor_chunk_offset, len);
			ptr += len;
			*ptr = 0;
			connection_bundle_advance_cursor(conn, len);
			return 0;
		}
	}
	*ptr = 0;
	return 0;
}

char connection_bundle_byte_at_relative_offset(struct conn_bundle *conn, int offset){
	struct linked_list *cursor;
	if(connection_bundle_reduce_cursor(conn)) return 0;
	if(!conn->cursor_chunk) return 0;
	cursor = conn->cursor_chunk;
	offset += conn->cursor_chunk_offset;
	if(offset < 0) return 0;
	while(cursor){
		if((size_t)offset < ((struct extent*)(cursor->data))->len)
			return ((struct extent*)(cursor->data))->bytes[offset];
		offset -= ((struct extent*)(cursor->data))->len;
		cursor = cursor->next;
	}
	return 0;
}

int connection_bundle_find_byte_offset_from(struct conn_bundle *conn, char target, int initial_offset){
	struct linked_list *cursor;
	struct extent *current = 0;
	int result = 0;
	if(initial_offset < 0) return -1;
	if(connection_bundle_reduce_cursor(conn)) return -1;
	cursor = conn->cursor_chunk;
	initial_offset += conn->cursor_chunk_offset;
	if(!cursor) return -1;
	current = (struct extent*)(cursor->data);
	while((size_t)initial_offset > current->len){
		initial_offset -= current->len;
		cursor = cursor->next;
		if(!cursor) return -1;
		current = (struct extent*)(cursor->data);
	}
	while(cursor){
		current = (struct extent*)(cursor->data);
		while((size_t)initial_offset < current->len){
			if(current->bytes[initial_offset] == target)
				return result;
			initial_offset++;
			result++;
			current = (struct extent*)(cursor->data);
		}
		current = (struct extent*)(cursor->data);
		initial_offset -= current->len;
		cursor = cursor->next;
	}
	return -1;
}

int connection_bundle_find_crlf_offset(struct conn_bundle *conn){
	int offset = 0;
	while(1){
		offset = connection_bundle_find_byte_offset_from(conn, '\r', offset);
		if(-1 == offset) return -1;
		if('\n' == connection_bundle_byte_at_relative_offset(conn, offset + 1))
			return offset + 2;
		offset++;
	}
}

int connection_bundle_consume_line(struct conn_bundle *conn){
	struct extent cursor;
	char *ptr = 0;
	int offset = connection_bundle_find_crlf_offset(conn);
	if(-1 == offset) return 1;
	if(!(conn->last_line)) conn->last_line = conn->lines;
	if(connection_bundle_take_bytes(conn, offset, &cursor)) return 2;
	ptr = palloc(conn->pool, sizeof(struct linked_list) + sizeof(struct extent));
	if(!conn->lines)
		conn->last_line = conn->lines = (struct linked_list*)ptr;
	else{
		conn->last_line->next = (struct linked_list*)ptr;
		conn->last_line = conn->last_line->next;
	}
	conn->last_line->next = 0;
	conn->last_line->data = ptr + sizeof(struct linked_list);
	((struct extent*)(conn->last_line->data))->bytes = cursor.bytes;
	((struct extent*)(conn->last_line->data))->len = cursor.len;
	printf("<%s>\n", cursor.bytes);
	return 0;
}

int connection_bundle_consume_method(struct conn_bundle *conn){
	int len;
	struct extent storage;
	if(conn->method) return 0;
	len = connection_bundle_find_byte_offset_from(conn, ' ', 0);
	if(-1 == len) return 1;
	if(connection_bundle_take_bytes(conn, len, &storage)) return 2;
	conn->method = (struct extent*)palloc(conn->pool, sizeof(struct extent));
	conn->method->bytes = storage.bytes;
	conn->method->len = storage.len;
	connection_bundle_advance_cursor(conn, 1);
	return 0;
}

int connection_bundle_consume_requrl(struct conn_bundle *conn){
	int len;
	struct extent storage;
	if(conn->request_url) return 0;
	len = connection_bundle_find_byte_offset_from(conn, ' ', 0);
	if(-1 == len) return 1;
	if(connection_bundle_take_bytes(conn, len, &storage)) return 2;
	conn->request_url = (struct extent*)palloc(conn->pool, sizeof(struct extent));
	conn->request_url->bytes = storage.bytes;
	conn->request_url->len = storage.len;
	connection_bundle_advance_cursor(conn, 1);
	return 0;
}

int connection_bundle_consume_http_version(struct conn_bundle *conn){
	int len;
	struct extent storage;
	int maj;
	if(-1 != conn->http_major_version) return 0;
	len = connection_bundle_find_byte_offset_from(conn, '/', 0);
	if(4 != len) return 1;
	if(connection_bundle_take_bytes(conn, 5, &storage)) return 2;
	if(strncmp(storage.bytes, "HTTP/", storage.len + 1)) return 3;
	len = connection_bundle_find_byte_offset_from(conn, '.', 0);
	if(-1 == len) return 4;
	if(len >= connection_bundle_find_crlf_offset(conn)) return 5;
	if(connection_bundle_take_bytes(conn, len, &storage)) return 6;
	maj = atoi(storage.bytes);
	connection_bundle_advance_cursor(conn, 1);
	len = connection_bundle_find_crlf_offset(conn);
	if(len < 2) return 6;
	if(connection_bundle_take_bytes(conn, len - 2, &storage)) return 7;
	conn->http_minor_version = atoi(storage.bytes);
	conn->http_major_version = maj;
	connection_bundle_advance_cursor(conn, 2);
	return 0;
}

int connection_bundle_can_respondp(struct conn_bundle *conn){
	if(!(conn->method)) return 0;
	if(!(conn->request_url)) return 0;
	if(-1 == conn->http_major_version) return 0;
	if(-1 == conn->http_minor_version) return 0;
	if(conn->done_writing) return 0;
	return 1;
}

int connection_bundle_free(struct conn_bundle *conn){
	if(conn->fd == -1) return 0;
	close(conn->fd);
	if(conn->pool){
		free_pool(conn->pool);
		conn->pool = 0;
	}
	memset(conn, 0, sizeof(struct conn_bundle));
	conn->done_writing = 1;
	conn->done_reading = 1;
	conn->fd = -1;
	conn->http_major_version = -1;
	conn->http_minor_version = -1;
	return 0;
}

int connection_bundle_close_write(struct conn_bundle *conn){
	if(conn->done_writing) return 0;
	conn->done_writing = 1;
	if(!conn->done_reading) return 0;
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
	if(!header->data) return 1;
	if(!header->next) return 1;
	if(!header->next->data) return 1;
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
	(void)node;
	/*
		https://tools.ietf.org/html/rfc7230#section-3.1.1
	*/
	if(!conn) return 0;
	if(!(conn->method)){
		if(connection_bundle_consume_method(conn)) return 0;
		*context = 1;
	}
	if(!(conn->request_url)){
		if(connection_bundle_consume_requrl(conn)) return 0;
		*context = 1;
	}
	if(-1 == conn->http_minor_version){
		if(connection_bundle_consume_http_version(conn)) return 0;
		*context = 1;
	}

	if(connection_bundle_can_respondp(conn)){
		*context = 1;
		return connection_bundle_respond(conn);
	}
	while(!connection_bundle_consume_line(conn)) *context = 1;
	connection_bundle_reduce_cursor(conn);
	return 0;
}
