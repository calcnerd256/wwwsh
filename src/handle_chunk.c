/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

/* included in manage_resources_forever.c */

int match_by_sockfd(struct conn_bundle *data, int *target, struct linked_list *node){
	(void)node;
	return data->fd == *target;
}

int connection_bundle_handle_chunk(struct conn_bundle *conn){
	char *buf;
	size_t len;
	struct extent *chunkptr;
	struct linked_list *new_head;
	if(!conn) return 2;
	buf = palloc(conn->pool, CHUNK_SIZE + 1 + sizeof(struct extent) + sizeof(struct linked_list));
	chunkptr = (struct extent*)(buf + CHUNK_SIZE + 1);
	new_head = (struct linked_list*)(buf + CHUNK_SIZE + 1 + sizeof(struct extent));
	new_head->next = conn->chunks;
	buf[CHUNK_SIZE] = 0;
	len = read(conn->fd, buf, CHUNK_SIZE);
	buf[len] = 0;
	chunkptr->bytes = buf;
	chunkptr->len = len;
	new_head->data = chunkptr;
	conn->chunks = new_head;
	printf("read %d bytes: <%s>\n", (int)chunkptr->len, chunkptr->bytes);
	if(!len){
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
	}
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
	if(first_matching(connections, (visitor_t)(&match_by_sockfd), (struct linked_list*)(&sockfd), &match_node))
		return 1;
	return connection_bundle_handle_chunk((struct conn_bundle*)(match_node->data));
}
