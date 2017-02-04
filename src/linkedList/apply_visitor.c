/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./apply_visitor.h"
#include "./linkedList.struct.h"
#include "./visitor_t.typedef.h"

#include "./string.inc.h"

int apply_visitor(char *fnptr_bytes, void* data, void* context, struct linked_list *node){
	visitor_t visitor;
	memcpy(&visitor, fnptr_bytes, sizeof(visitor_t));
	return (*visitor)(data, context, node);
}
