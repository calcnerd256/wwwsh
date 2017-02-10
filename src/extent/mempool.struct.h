/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#ifndef INCLUDE_mempool_STRUCT
# define INCLUDE_mempool_STRUCT

# include "./linkedList.struct.h"

struct mempool{
	struct linked_list *allocs;
};

#endif
