/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./free_pool.h"
#include "./mempool.struct.h"
#include "./clean_and_free_list.h"
#include "./visitor_t.typedef.h"
#include "./visit_clear.h"

int free_pool(struct mempool *pool){
	return clean_and_free_list(pool->allocs, (visitor_t)&visit_clear, 0);
}
