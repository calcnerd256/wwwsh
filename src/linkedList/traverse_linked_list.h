/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#ifndef INCLUDE_traverseLinkedList
#define INCLUDE_traverseLinkedList

#include "./linkedList.struct.h"
#include "./visitor_t.typedef.h"

int traverse_linked_list(
	struct linked_list *head,
	visitor_t visitor,
	void *context
);

#endif
