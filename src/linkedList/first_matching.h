/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#ifndef INCLUDE_firstMatching
#define INCLUDE_firstMatching

#include "./linkedList.struct.h"
#include "./visitor_t.typedef.h"

int first_matching(
	struct linked_list *head,
	visitor_t matcher,
	struct linked_list *context,
	struct linked_list* *out_match
);

#endif
