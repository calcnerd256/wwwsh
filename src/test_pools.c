/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

int test_pools(){
	struct mempool pool;
	char *str;
	if(init_pool(&pool)) return 1;
	str = palloc(&pool, 3);
	str[2] = 0;
	str[0] = 'O';
	str[1] = 'K';
	printf("%p={allocs: %p=<%p[:%d]=%p=%p=\"%s\", %p>}\n", &pool, pool.allocs, pool.allocs->data, ((struct extent*)(pool.allocs->data))->len, ((struct extent*)(pool.allocs->data))->bytes, str, str, pool.allocs->next);
	free_pool(&pool);
	return 0;
}
