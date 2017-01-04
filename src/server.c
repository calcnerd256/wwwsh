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

