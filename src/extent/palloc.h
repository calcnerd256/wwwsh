/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#ifndef INCLUDE_palloc
# define INCLUDE_palloc

# include "./mempool.struct.h"
# include "./size_t.typedef.h"

void *palloc(
	struct mempool *pool,
	size_t len
);

#endif
