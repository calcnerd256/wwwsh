/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

/* included in manage_resources_forever.c */

int match_by_sockfd(struct conn_bundle *data, int *target, struct linked_list *node){
	(void)node;
	return data->fd == *target;
}

int connection_bundle_close(struct conn_bundle *conn){
	struct linked_list *current = 0;
	conn->done_reading = 1;
	printf("request socket with file descriptor %d closed; body follows:\n", conn->fd);
	current = conn->lines;
	while(current){
		printf("%s", ((struct extent*)(current->data))->bytes);
		current = current->next;
	}
	printf("\n\nDone.\n");
	/*
		printf("by line:\n");
		traverse_linked_list(conn->lines->next, (visitor_t)&visit_print_ransom, (void*)1);
		printf("\n\nDone.\n");
	*/
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
	return connection_bundle_close(conn);
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

int connection_bundle_find_crlf_offset(struct conn_bundle *conn){
	struct extent *current;
	char found_cr = 0;
	size_t offset = 0;
	struct linked_list *cursor = conn->cursor_chunk;
	size_t coff = conn->cursor_chunk_offset;
	if(!cursor) cursor = conn->chunks;
	if(!cursor) return -1;
	current = (struct extent*)(cursor->data);
	while(1){
		while(coff >= current->len){
			if(!cursor->next) return -1;
			coff -= current->len;
			cursor = cursor->next;
			current = (struct extent*)(cursor->data);
		}
		if(found_cr){
			found_cr = 0;
			if('\n' == current->bytes[coff])
				return offset + 1;
		}
		found_cr = '\r' == current->bytes[coff];
		offset++;
		coff++;
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
	return 0;
}

int connection_bundle_process_step(struct conn_bundle *conn){
	while(!connection_bundle_consume_line(conn))
		printf("%d-byte line: %s", (int)(((struct extent*)(conn->last_line->data))->len), ((struct extent*)(conn->last_line->data))->bytes);
	connection_bundle_reduce_cursor(conn);
	printf("partial line:<%s>\n", ((struct extent*)(conn->cursor_chunk->data))->bytes + conn->cursor_chunk_offset);
	/*
	if(parse_lines_from_chunk(conn->lines, buf)) return 3;
	if(parse_from_lines(conn)){
		printf("failed to parse HTTP request");
		return 4;
	}
	*/
	return 0;
}

int handle_chunk(int sockfd, struct linked_list *connections){
	struct linked_list *match_node;
	struct conn_bundle *connection;
	if(first_matching(connections, (visitor_t)(&match_by_sockfd), (struct linked_list*)(&sockfd), &match_node))
		return 1;
	connection = (struct conn_bundle*)(match_node->data);
	if(connection_bundle_sip(connection)) return 2;
	return connection_bundle_process_step(connection);
}
