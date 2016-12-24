/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

/*
	included in main.c
	match_by_sockfd is called by handle_chunk as a visitor
*/

int match_by_sockfd(struct conn_bundle *data, int *target, struct linked_list *node){
	(void)node;
	return data->fd == *target;
}


/*

	find the connection attached to the socket
	contiguously allocate the extent for the chunk, the list node pointing to it, and the buffer of its bytes
	read the bytes into the buffer, point the extent at it, and append this to the connection's chunk list
	if the chunk list is empty, appending is assignment of the node to the list's head
	if the read is empty, the socket has been closed, so close the read end of the connection

*/

int handle_chunk(int sockfd, struct linked_list *connections){
	struct linked_list *match_node;
	struct conn_bundle *conn;
	char *buf;
	size_t len;
	struct extent *chunkptr;
	struct linked_list *new_head;

	if(first_matching(connections, (visitor_t)(&match_by_sockfd), (struct linked_list*)(&sockfd), &match_node))
		return 1;
	conn = (struct conn_bundle*)(match_node->data);
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
	conn->done_reading = 1;
	if(conn->done_writing)
		close(conn->fd);
	return 0;
}
