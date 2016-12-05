/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./lines.h"

struct mempool{
	struct linked_list *allocs;
};

int init_pool(struct mempool *pool);
void *palloc(struct mempool *pool, size_t len);
int visit_free(struct extent *data, void *context, struct linked_list *node);
int free_pool(struct mempool *pool);
