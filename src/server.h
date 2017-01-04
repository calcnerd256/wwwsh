/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./server_structs.h"

int httpServer_init(struct httpServer *server, struct mempool *pool);

int httpServer_pushResource(
	struct httpServer *server,
	struct linked_list *new_head,
	struct httpResource *resource,
	int (*urlMatchesp)(struct httpResource*, struct extent*),
	int (*respond)(struct httpResource*, struct conn_bundle*),
	void *context
);
