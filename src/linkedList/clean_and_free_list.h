/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#ifndef INCLUDE_cleanAndFreeList
#define INCLUDE_cleanAndFreeList

#include "./linkedList.struct.h"
#include "./visitor_t.typedef.h"

int clean_and_free_list(
	struct linked_list *head,
	visitor_t cleaner,
	void *context
);

#endif
