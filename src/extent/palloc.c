/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

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
