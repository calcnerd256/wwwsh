/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#ifndef INCLUDE_visitor
#define INCLUDE_visitor

#include "./linkedList.struct.h"

typedef int (*visitor_t)(
	void*,
	void*,
	struct linked_list*
);

#endif
