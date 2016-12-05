/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <stdlib.h>
#include <string.h>
#include "./mempool.h"

int init_pool(struct mempool *pool){
	pool->allocs = 0;
	return 0;
}

void *palloc(struct mempool *pool, size_t len){
	char *new_head = malloc(sizeof(struct extent) + sizeof(struct linked_list) + len);
	((struct linked_list*)new_head)->next = pool->allocs;
	pool->allocs = (struct linked_list*)new_head;
	new_head += sizeof(struct linked_list);
	pool->allocs->data = new_head;
	((struct extent*)new_head)->len = len;
	((struct extent*)new_head)->bytes = new_head + sizeof(struct extent);
	return ((struct extent*)new_head)->bytes;
}

int visit_nop(void *data, void *context, struct linked_list *node){
	(void)data;
	(void)context;
	(void)node;
	return 0;
}
int visit_clear(struct extent *data, void *context, struct linked_list *node){
	(void)context;
	if(!data) return 1;
	if(!(data->bytes)) return 0;
	if(!(data->len)) return 0;
	memset(data->bytes, 0, data->len);
	data->len = 0;
	data->bytes = 0;
	node->data = 0;
	return 0;
}

int free_pool(struct mempool *pool){
	return clean_and_free_list(pool->allocs, (visitor_t)&visit_clear, 0);
}

int pool_pop(struct mempool *pool){
	struct linked_list *head = pool->allocs;
	pool->allocs = head->next;
	head->next = 0;
	if(visit_clear((struct extent*)(head->data), 0, head)) return 1;
	free(head);
	return 0;
}
