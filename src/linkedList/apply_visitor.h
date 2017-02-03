/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#ifndef INCLUDE_applyVisitor
#define INCLUDE_applyVisitor

#include "./visitor_t.typedef.h"

int apply_visitor(
	char *fnptr_bytes,
	void* data,
	void* context,
	struct linked_list *node
);

#endif
