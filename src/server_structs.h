/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

/*

  Unfortunately, these structures and their method signatures reference each other.

*/

struct httpServer{
	struct mempool *memoryPool;
	struct linked_list *connections;
	struct linked_list *resources;
	int listeningSocket_fileDescriptor;
};

struct staticGetResource{
	struct extent *url;
	struct extent *body;
	struct linked_list *headers;
};

struct conn_bundle{
	struct requestInput input;
	struct mempool allocations;
	struct httpServer *server;
	struct linked_list *node;
	int fd;
	char done_writing;
};

struct httpResource{
	int (*urlMatchesp)(struct httpResource*, struct extent*);
	int (*respond)(struct httpResource*, struct conn_bundle*);
	void *context;
};
