/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

/*

  Unfortunately, these structures and their method signatures reference each other.

*/

#ifndef INCLUDE_SERVER_STRUCTS
#define INCLUDE_SERVER_STRUCTS

#include "./requestInput.h"

struct httpServer{
	struct linked_list *connections;
	struct linked_list *resources;
	struct linked_list *children;
	unsigned long int nextChild;
	int listeningSocket_fileDescriptor;
};

struct incomingHttpRequest{
	struct requestInput input;
	struct mempool allocations;
	struct httpServer *server;
	struct linked_list *node;
	int fd;
	char done_writing;
};

struct httpResource{
	struct extent url;
	int (*urlMatchesp)(struct httpResource*, struct extent*);
	int (*canRespondp)(struct httpResource*, struct incomingHttpRequest*);
	int (*respond)(struct httpResource*, struct incomingHttpRequest*);
	void *context;
};

#endif
