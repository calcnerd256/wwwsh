/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./server.h"

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
