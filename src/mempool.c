/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <stdlib.h>
#include "./mempool.h"

int init_pool(struct mempool *pool){
	pool->allocs = 0;
	return 0;
}

void *palloc(struct mempool *pool, size_t len){
	char *new_head = malloc(sizeof(struct extent) + sizeof(struct linked_list));
	((struct linked_list*)new_head)->next = pool->allocs;
	pool->allocs = (struct linked_list*)new_head;
	new_head += sizeof(struct linked_list);
	pool->allocs->data = new_head;
	((struct extent*)new_head)->bytes = malloc(len);
	((struct extent*)new_head)->len = len;
	return ((struct extent*)new_head)->bytes;
}

int visit_free(struct extent *data, void *context, struct linked_list *node){
	(void)node;
	(void)context;
	if(!data) return 1;
	if(!(data->bytes)) return 0;
	if(!(data->len)) return 0;
	free(data->bytes);
	data->bytes = 0;
	node->data = 0;
	return 0;
}

int free_pool(struct mempool *pool){
	return clean_and_free_list(pool->allocs, (visitor_t)&visit_free, 0);
}
