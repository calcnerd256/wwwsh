/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./init_pool.h"
#include "./mempool.struct.h"

int init_pool(struct mempool *pool){
	pool->allocs = 0;
	return 0;
}
