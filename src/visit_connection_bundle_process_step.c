/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

/* included in main.c */

int connection_bundle_can_respondp(struct conn_bundle *conn){
	if(!(conn->input.method)) return 0;
	if(!(conn->input.requestUrl)) return 0;
	if(-1 == conn->input.httpMajorVersion) return 0;
	if(-1 == conn->input.httpMinorVersion) return 0;
	if(conn->done_writing) return 0;
	return 1;
}

int connection_bundle_free(struct conn_bundle *conn){
	if(conn->fd == -1) return 0;
	close(conn->fd);
	conn->fd = -1;

	if(conn->allocations.allocs){

		requestInput_consumeLastLine(&(conn->input));

		free_pool(&(conn->allocations));
		memset(&(conn->allocations), 0, sizeof(struct mempool));
	}
	memset(conn, 0, sizeof(struct conn_bundle));
	conn->done_writing = 1;
	conn->input.done = 1;
	conn->input.httpMajorVersion = -1;
	conn->input.httpMinorVersion = -1;
	return 0;
}

int connection_bundle_close_write(struct conn_bundle *conn){
	if(conn->done_writing) return 0;
	conn->done_writing = 1;
	if(!(conn->input.done)) return 0;
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

int staticGetResource_urlMatchesp(struct httpResource *resource, struct extent *url){
	if(!resource) return 0;
	if(!(resource->context)) return 0;
	if(!url) return 0;
	return match_resource_url(resource->context, url, 0);
}

int match_httpResource_url(struct httpResource *resource, struct extent *url, struct linked_list *node){
	(void)node;
	node = 0;
	if(!url) return 0;
	if(!resource) return 0;
	if(!(resource->urlMatchesp)) return 0;
	return (*(resource->urlMatchesp))(resource, url);
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
	struct staticGetResource *resource;
	struct extent reason;
	struct linked_list *match_node = 0;
	if(conn->done_writing) return 0;

	resource = 0;
	if(!first_matching(conn->server->staticResources, (visitor_t)(&match_resource_url), (struct linked_list*)(conn->input.requestUrl), &match_node))
		if(match_node)
			resource = match_node->data;
	if(!resource)
		return connection_bundle_respond_bad_request_target(conn);

	if(strncmp(conn->input.method->bytes, "GET", conn->input.method->len + 1))
		return connection_bundle_respond_bad_method(conn);
	point_extent_at_nice_string(&reason, "OK");
	return connection_bundle_send_response(conn, 200, &reason, resource->headers, resource->body);
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
