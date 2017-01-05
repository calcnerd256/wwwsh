/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "./server.h"
#include "./network.h"
#include "./request.h"


int httpServer_init(struct httpServer *server, struct mempool *pool){
	server->listeningSocket_fileDescriptor = -1;
	server->memoryPool = pool;
	init_pool(server->memoryPool);
	server->connections = 0;
	server->resources = 0;
	server = 0;
	return 0;
}

int httpServer_pushResource(struct httpServer *server, struct linked_list *new_head, struct httpResource *resource, int (*urlMatchesp)(struct httpResource*, struct extent*), int (*respond)(struct httpResource*, struct conn_bundle*), void *context){
	if(!server) return 1;
	if(!new_head) return 2;
	if(!urlMatchesp) return 2;
	if(!respond) return 2;
	resource->urlMatchesp = urlMatchesp;
	resource->respond = respond;
	resource->context = context;
	new_head->next = server->resources;
	new_head->data = resource;
	server->resources = new_head;
	return 0;
}

int httpServer_listen(struct httpServer *server, char* port_number, int backlog){
	int sockfd = -1;
	if(get_socket(port_number, &sockfd)){
		server = 0;
		port_number = 0;
		backlog = 0;
		sockfd = 0;
		return 1;
	}
	port_number = 0;
	server->listeningSocket_fileDescriptor = sockfd;
	server = 0;
	if(listen(sockfd, backlog)){
		backlog = 0;
		sockfd = 0;
		return 2;
	}
	sockfd = 0;
	backlog = 0;
	return 0;
}

/* TODO: consider cleaning up all open connections when closing the server */
int httpServer_close(struct httpServer *server){
	if(server->memoryPool){
		free_pool(server->memoryPool);
		free(server->memoryPool);
		server->memoryPool = 0;
	}
	server->resources = 0;
	if(-1 == server->listeningSocket_fileDescriptor){
		server = 0;
		return 0;
	}
	if(shutdown(server->listeningSocket_fileDescriptor, SHUT_RDWR)){
		server = 0;
		return 1;
	}
	if(close(server->listeningSocket_fileDescriptor)){
		server = 0;
		return 2;
	}
	server->listeningSocket_fileDescriptor = -1;
	server = 0;
	return 0;
}


int visit_connection_bundle_select_read(struct conn_bundle *conn, struct linked_list *context, struct linked_list *node){
	int *done;
	int *fd;
	int status;
	(void)node;
	if(!context) return 1;
	if(!context->next) return 1;
	done = (int *)(context->data);
	if(!done) return 3;
	if(*done) return 0;
	fd = (int *)(context->next->data);
	if(!fd) return 3;
	status = incomingHttpRequest_selectRead(conn);
	if(-1 == status) return 0;
	*fd = status;
	*done = 1;
	return 0;
}
int httpServer_selectRead(struct httpServer *server){
	int ready_fd;
	struct linked_list context;
	struct linked_list fd_cell;
	struct conn_bundle fake_for_server;
	struct linked_list extra_head;
	int done;
	done = 0;
	ready_fd = -1;
	fd_cell.next = 0;
	fd_cell.data = &ready_fd;
	context.next = &fd_cell;
	context.data = &done;
	extra_head.next = server->connections;
	extra_head.data = &fake_for_server;
	fake_for_server.fd = server->listeningSocket_fileDescriptor;
	fake_for_server.input.done = 0;
	if(traverse_linked_list(&extra_head, (visitor_t)(&visit_connection_bundle_select_read), &context)) return -1;
	return ready_fd;
}

int httpServer_acceptNewConnection(struct httpServer *server){
	struct sockaddr address;
	socklen_t length;
	int fd;
	struct linked_list *new_head;

	memset(&address, 0, sizeof(struct sockaddr));
	length = 0;
	fd = accept(server->listeningSocket_fileDescriptor, &address, &length);
	if(-1 == fd) return 1;

	new_head = malloc(sizeof(struct linked_list));
	new_head->data = malloc(sizeof(struct conn_bundle));
	new_head->next = server->connections;
	init_connection((struct conn_bundle*)(new_head->data), server, fd);
	((struct conn_bundle*)(new_head->data))->node = new_head;
	server->connections = new_head;
	return 0;
}

int httpServer_removeEmptyConnections(struct httpServer *server){
	struct linked_list *node = 0;
	struct linked_list *middle = 0;
	if(!server) return 1;
	if(!(server->connections)) return 0;
	while(server->connections && !(server->connections->data)){
		node = server->connections->next;
		server->connections->next = 0;
		free(server->connections);
		server->connections = node;
	}
	node = server->connections;
	while(node){
		while(node->next && !(node->next->data)){
			middle = node->next;
			node->next = middle->next;
			middle->next = 0;
			free(middle);
		}
		node = node->next;
	}
	return 0;
}


int match_httpResource_url(struct httpResource *resource, struct extent *url, struct linked_list *node){
	(void)node;
	node = 0;
	if(!url) return 0;
	if(!resource) return 0;
	if(!(resource->urlMatchesp)) return 0;
	return (*(resource->urlMatchesp))(resource, url);
}


int visit_connection_bundle_process_step(struct conn_bundle *conn, int *context, struct linked_list *node){
	(void)node;
	if(connection_bundle_process_steppedp(conn)) *context = 1;
	conn = 0;
	context = 0;
	node = 0;
	return 0;
}
int httpServer_stepConnections(struct httpServer *server){
	int any = 0;
	if(traverse_linked_list(server->connections, (visitor_t)(&visit_connection_bundle_process_step), &any)) return 0;
	httpServer_removeEmptyConnections(server);
	return any;
}