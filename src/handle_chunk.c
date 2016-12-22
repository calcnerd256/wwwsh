/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

/* included in manage_resources_forever.c */

int match_by_sockfd(struct conn_bundle *data, int *target, struct linked_list *node){
	(void)node;
	return data->fd == *target;
}

int connection_bundle_close_read(struct conn_bundle *conn){
	conn->done_reading = 1;
	if(conn->done_writing)
		close(conn->fd);
	return 0;
}

int connection_bundle_sip(struct conn_bundle *conn){
	char *buf;
	size_t len;
	struct extent *chunkptr;
	struct linked_list *new_head;
	if(!conn) return 2;
	buf = palloc(conn->pool, CHUNK_SIZE + 1 + sizeof(struct extent) + sizeof(struct linked_list));
	chunkptr = (struct extent*)(buf + CHUNK_SIZE + 1);
	new_head = (struct linked_list*)(buf + CHUNK_SIZE + 1 + sizeof(struct extent));
	new_head->next = 0;
	buf[CHUNK_SIZE] = 0;
	len = read(conn->fd, buf, CHUNK_SIZE);
	buf[len] = 0;
	chunkptr->bytes = buf;
	chunkptr->len = len;
	new_head->data = chunkptr;
	*(conn->last_chunk ? &(conn->last_chunk->next) : &(conn->chunks)) = new_head;
	conn->last_chunk = new_head;
	conn->request_length += len;
	if(len) return 0;
	return connection_bundle_close_read(conn);
}

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
	int result = 0;
	if(initial_offset < 0) return -1;
	if(connection_bundle_reduce_cursor(conn)) return -1;
	cursor = conn->cursor_chunk;
	initial_offset += conn->cursor_chunk_offset;
	if(!cursor) return -1;
	while((size_t)initial_offset > ((struct extent*)(cursor->data))->len){
		initial_offset -= ((struct extent*)(cursor->data))->len;
		cursor = cursor->next;
		if(!cursor) return -1;
	}
	while(cursor){
		while((size_t)initial_offset < ((struct extent*)(cursor->data))->len){
			if(((struct extent*)(cursor->data))->bytes[initial_offset] == target)
				return result;
			initial_offset++;
			result++;
		}
		initial_offset -= ((struct extent*)(cursor->data))->len;
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

int connection_bundle_close_write(struct conn_bundle *conn){
	conn->done_writing = 1;
	if(conn->done_reading)
		close(conn->fd);
	return 0;
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

int connection_bundle_respond_bad_method(struct conn_bundle *conn){
	ssize_t bytes;
	struct extent reason;
	struct extent body;
	struct extent allow;
	struct extent get;
	struct extent content_length_key;
	struct extent content_length_value;
	struct extent connection;
	struct extent close;
	char content_length_str[256];
	int status_code = 405;
	reason.bytes = "METHOD NOT ALLOWED";
	reason.len = 18;
	body.bytes = "Method Not Allowed\r\nOnly GET requests are accepted here.\r\n\r\n";
	body.len = 60;
	allow.bytes = "Allow";
	allow.len = 5;
	get.bytes = "GET";
	get.len = 3;
	content_length_key.bytes = "Content-Length";
	content_length_key.len = 14;
	snprintf(content_length_str, 255, "%d", (int)body.len);
	content_length_str[255] = 0;
	content_length_value.bytes = content_length_str;
	content_length_value.len = strlen(content_length_str);
	connection.bytes = "Connection";
	connection.len = 10;
	close.bytes = "close";
	close.len = 5;
	if(connection_bundle_write_status_line(conn, status_code, &reason)) return 1;
	if(connection_bundle_write_header(conn, &allow, &get)) return 1;
	if(connection_bundle_write_header(conn, &content_length_key, &content_length_value)) return 1;
	if(connection_bundle_write_header(conn, &connection, &close)) return 1;
	if(connection_bundle_write_crlf(conn)) return 1;
	bytes = write(conn->fd, body.bytes, body.len);
	if(bytes < 0) return 1;
	if((size_t)bytes != body.len) return 1;
	return connection_bundle_close_write(conn);
}
int connection_bundle_respond_bad_request_target(struct conn_bundle *conn){
	ssize_t bytes = write(conn->fd, "HTTP/1.1 404 NOT FOUND\r\nContent-Length: 34\r\nConnection: close\r\n\r\nNot Found\r\nOnly / exists here.\r\n\r\n", 99);
	if(bytes != 99) return 1;
	return connection_bundle_close_write(conn);
}
int connection_bundle_respond_root(struct conn_bundle *conn){
	ssize_t bytes = write(conn->fd, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 192\r\nConnection: close\r\n\r\n<html>\r\n <head>\r\n  <title>Hello World!</title>\r\n </head>\r\n <body>\r\n  <h1>Hello, World!</h1>\r\n  <p>\r\n   This webserver is written in C.\r\n  I'm pretty proud of it!\r\n  </p>\r\n </body>\r\n</html>\r\n\r\n", 276);
	if(276 != bytes) return 1;
	return connection_bundle_close_write(conn);
}
int connection_bundle_respond(struct conn_bundle *conn){
	if(conn->done_writing) return 0;
	if(strncmp(conn->method->bytes, "GET", conn->method->len + 1))
		return connection_bundle_respond_bad_method(conn);
	if(strncmp(conn->request_url->bytes, "/", conn->request_url->len + 1))
		return connection_bundle_respond_bad_request_target(conn);
	return connection_bundle_respond_root(conn);
}

int visit_connection_bundle_process_step(struct conn_bundle *conn, void* context, struct linked_list *node){
	(void)context;
	(void)node;
	/*
		https://tools.ietf.org/html/rfc7230#section-3.1.1
	*/
	if(connection_bundle_consume_method(conn)) return 0;
	if(connection_bundle_consume_requrl(conn)) return 0;
	if(connection_bundle_consume_http_version(conn)) return 0;

	if(connection_bundle_can_respondp(conn))
		return connection_bundle_respond(conn);
	while(!connection_bundle_consume_line(conn));
	connection_bundle_reduce_cursor(conn);
	return 0;
}

int handle_chunk(int sockfd, struct linked_list *connections){
	struct linked_list *match_node;
	struct conn_bundle *connection;
	if(first_matching(connections, (visitor_t)(&match_by_sockfd), (struct linked_list*)(&sockfd), &match_node))
		return 1;
	connection = (struct conn_bundle*)(match_node->data);
	return connection_bundle_sip(connection);
}
