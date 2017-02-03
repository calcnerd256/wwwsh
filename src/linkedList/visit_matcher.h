/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#ifndef INCLUDE_visitMatcher
#define INCLUDE_visitMatcher

#include "./linkedList.struct.h"

int visit_matcher(
	void *data,
	struct linked_list *context,
	struct linked_list *node
);

#endif
