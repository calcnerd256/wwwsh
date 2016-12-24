/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

/* included in main.c */

int push_without_alloc(struct linked_list* *head, struct linked_list *storage, void* data){
	storage->data = data;
	storage->next = *head;
	*head = storage;
	return 0;
}

int accept_new_connection(int server_socket, struct mempool *allocations, struct linked_list* *head){
	struct sockaddr address;
	socklen_t length;
	int fd;
	char *ptr;

	if(!head) return 1;
	memset(&address, 0, sizeof(struct sockaddr));
	length = 0;
	fd = accept(server_socket, &address, &length);
	if(-1 == fd) return 1;

	ptr = palloc(allocations, sizeof(struct conn_bundle) + sizeof(struct linked_list));
	push_without_alloc(head, (struct linked_list*)ptr, ptr + sizeof(struct linked_list));
	init_connection((struct conn_bundle*)((*head)->data), allocations, fd);
	return 0;
}
