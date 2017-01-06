/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "./server.h"
#include "./network.h"
#include "./request.h"


int httpServer_init(struct httpServer *server){
	server->listeningSocket_fileDescriptor = -1;
	server->connections = 0;
	server->resources = 0;
	server = 0;
	return 0;
}

int httpServer_pushResource(struct httpServer *server, struct linked_list *new_head, struct httpResource *resource, int (*urlMatchesp)(struct httpResource*, struct extent*), int (*respond)(struct httpResource*, struct incomingHttpRequest*), void *context){
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

int httpServer_canAcceptConnectionp(struct httpServer *server){
	struct incomingHttpRequest fake;
	fake.fd = server->listeningSocket_fileDescriptor;
	fake.input.done = 0;
	return -1 != incomingHttpRequest_selectRead(&fake);
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
	new_head->data = malloc(sizeof(struct incomingHttpRequest));
	new_head->next = server->connections;
	incomingHttpRequest_init((struct incomingHttpRequest*)(new_head->data), server, fd);
	((struct incomingHttpRequest*)(new_head->data))->node = new_head;
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
struct httpResource* httpServer_locateResource(struct httpServer *server, struct extent *url){
	struct httpResource *retval = 0;
	struct linked_list *match_node = 0;
	struct linked_list *head = server->resources;
	visitor_t matcher = (visitor_t)(&match_httpResource_url);
	int result = first_matching(head, matcher, (struct linked_list*)url, &match_node);
	server = 0;
	url = 0;
	head = 0;
	matcher = 0;
	if(result){
		match_node = 0;
		result = 0;
		return 0;
	}
	result = 0;
	if(!match_node){
		match_node = 0;
		return 0;
	}
	retval = match_node->data;
	match_node = 0;
	return retval;
}


int visit_connection_bundle_process_step(struct incomingHttpRequest *conn, int *context, struct linked_list *node){
	(void)node;
	if(incomingHttpRequest_processSteppedp(conn)) *context = 1;
	conn = 0;
	context = 0;
	node = 0;
	return 0;
}
int httpServer_stepConnections(struct httpServer *server){
	int any = 0;
	httpServer_removeEmptyConnections(server);
	if(traverse_linked_list(server->connections, (visitor_t)(&visit_connection_bundle_process_step), &any)) return 0;
	httpServer_removeEmptyConnections(server);
	return any;
}
