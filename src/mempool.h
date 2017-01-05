/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <sys/types.h>
#include "./linked_list.h"


struct extent{
	char *bytes;
	size_t len;
};

int point_extent_at_nice_string(struct extent *storage, char *bytes);


struct mempool{
	struct linked_list *allocs;
};

int init_pool(struct mempool *pool);
void *palloc(struct mempool *pool, size_t len);
int free_pool(struct mempool *pool);
