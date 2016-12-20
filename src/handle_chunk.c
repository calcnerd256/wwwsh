/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

/* included in manage_resources_forever.c */

int match_by_sockfd(struct conn_bundle *data, int *target, struct linked_list *node){
	(void)node;
	return data->fd == *target;
}

int connection_bundle_close(struct conn_bundle *conn){
	conn->done_reading = 1;
	printf("request socket with file descriptor %d closed\n", conn->fd);
		/*
		printf("request socket with file descriptor %d closed; body follows:\n", conn->fd);
		traverse_linked_list(conn->request->next, (visitor_t)(&visit_print_string), 0);
		printf("\n\nDone.\n");
		printf("by line:\n");
		traverse_linked_list(conn->lines->next, (visitor_t)&visit_print_ransom, (void*)1);
		printf("\n\nDone.\n");
		memset(buf, 0, CHUNK_SIZE+1);
		free(buf);
		return 0;
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
	printf("read %d more bytes (totaling %d): <%s>\n", (int)chunkptr->len, (int)conn->request_length, chunkptr->bytes);
	if(len) return 0;
	return connection_bundle_close(conn);
}

/*
int connection_bundle_get_cursor(struct conn_bundle *conn, struct extent *cursor_out){
	struct extent *current_chunk;
	unsigned long int length_remaining = conn->request_length - conn->cursor_position;
	struct linked_list *head = conn->chunks;
	while(length_remaining > 0){
		if(!head) return 1;
		current_chunk = (struct extent*)(head->data);
		if(length_remaining > current_chunk->len)
			length_remaining -= current_chunk->len;
		else{
			cursor_out->bytes = current_chunk->bytes + current_chunk->len - length_remaining;
			cursor_out->len = length_remaining;
			return 0;
		}
		head = head->next;
	}
	return 1;
}

int connection_bundle_take_bytes(struct conn_bundle *conn, size_t len, struct extent *buf_out){
	struct extent cursor;
	char *ptr = 0;
	if(connection_bundle_get_cursor(conn, &cursor)) return 1;
	if(cursor.len >= len){
		buf_out->len = len;
		buf_out->bytes = cursor.bytes;
		conn->cursor_position += len;
		return 0;
	}
	buf_out->len = len;
	buf_out->bytes = palloc(conn->pool, len);
	memset(buf_out->bytes, 0, len);
	memcpy(buf_out->bytes, cursor.bytes, cursor.len);
	ptr = buf_out->bytes + cursor.len;
	len -= cursor.len;
	conn->cursor_position += cursor.len;
	*ptr = 0;
	return 1;
}
*/

int connection_bundle_process_step(struct conn_bundle *conn){
	/*
	struct extent cursor;
	if(connection_bundle_take_bytes(conn, 19, &cursor)) return 1;
	printf("%d bytes available in <%s>\n", (int)(cursor.len), cursor.bytes);
	*/
	/*
	if(parse_lines_from_chunk(conn->lines, buf)) return 3;
	if(parse_from_lines(conn)){
		printf("failed to parse HTTP request");
		return 4;
	}
	*/
	(void)conn;
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
