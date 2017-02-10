/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./palloc.h"
#include "./mempool.struct.h"
#include "./size_t.typedef.h"
#include "./malloc.h"
#include "./extent.struct.h"
#include "./linkedList.struct.h"

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
