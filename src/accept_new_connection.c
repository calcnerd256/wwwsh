/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

/* included in main.c */

int httpServer_acceptNewConnection(struct httpServer *server){
	struct sockaddr address;
	socklen_t length;
	int fd;
	char *ptr;

	memset(&address, 0, sizeof(struct sockaddr));
	length = 0;
	fd = accept(server->listeningSocket_fileDescriptor, &address, &length);
	if(-1 == fd) return 1;

	ptr = palloc(server->memoryPool, sizeof(struct conn_bundle) + sizeof(struct linked_list));
	push_without_alloc(&(server->connections), (struct linked_list*)ptr, ptr + sizeof(struct linked_list));
	init_connection((struct conn_bundle*)(server->connections->data), server->memoryPool, fd);
	return 0;
}
