/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

/*
	included in manage_resources_forever.c
*/


int visit_connection_set_fds_and_max(struct conn_bundle *conn, struct linked_list *context, struct linked_list *node){
	fd_set *to_read;
	int *maxfd;
	(void)node;
	if(conn->done_reading) return 0;
	to_read = (fd_set*)(context->data);
	maxfd = (int*)(context->next->data);
	FD_SET(conn->fd, to_read);
	if(*maxfd < conn->fd)
		*maxfd = conn->fd;
	return 0;
}

int set_fds_and_max_from_connections(fd_set *to_read, int *maxfd, struct linked_list *connections){
	struct linked_list context[2];
	context[0].data = (void*)to_read;
	context[1].data = (void*)maxfd;
	context[0].next = &(context[1]);
	context[1].next = 0;
	return traverse_linked_list(connections, (visitor_t)(&visit_connection_set_fds_and_max), context);
}

int visit_connection_fds_isset_status(struct conn_bundle *conn, struct linked_list *context, struct linked_list *node){
	fd_set *to_read;
	int *out_fd;
	int *found;
	(void)node;
	to_read = (fd_set*)(context->data);
	out_fd = (int*)(context->next->data);
	found = (int*)(context->next->next->data);
	if(*found) return 0;
	if(!FD_ISSET(conn->fd, to_read)) return 0;
	*found = 1;
	*out_fd = conn->fd;
	return 0;
}

int search_for_isset_fd_from_connections(fd_set *to_read, int *out_fd, struct linked_list *connections){
	struct linked_list context[3];
	int status = 0;
	context[0].data = (void*)to_read;
	context[1].data = (void*)out_fd;
	context[2].data = (void*)&status;
	context[0].next = &(context[1]);
	context[1].next = &(context[2]);
	context[2].next = 0;
	traverse_linked_list(connections, (visitor_t)(&visit_connection_fds_isset_status), context);
	return status;
}


int await_a_resource(int listening_socket, struct timeval *timeout, int *out_fd, struct linked_list *connections){
	fd_set to_read;
	int status;
	int maxfd;
	FD_ZERO(&to_read);
	FD_SET(listening_socket, &to_read);
	maxfd = listening_socket;
	set_fds_and_max_from_connections(&to_read, &maxfd, connections);
	status = select(maxfd + 1, &to_read, 0, 0, timeout);
	if(-1 == status){
		FD_ZERO(&to_read);
		return 2;
	}
	if(!status){
		FD_ZERO(&to_read);
		return 1;
	}
	if(FD_ISSET(listening_socket, &to_read)){
		FD_ZERO(&to_read);
		*out_fd = listening_socket;
		return 0;
	}
	status = search_for_isset_fd_from_connections(&to_read, out_fd, connections);
	FD_ZERO(&to_read);
	if(status)
		return 0;
	return 3;
}
