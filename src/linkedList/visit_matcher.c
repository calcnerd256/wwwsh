/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./visit_matcher.h"
#include "./linkedList.struct.h"
#include "./apply_visitor.h"

int visit_matcher(void *data, struct linked_list *context, struct linked_list *node){
	char *matcher;
	struct linked_list* *result;
	matcher = (char*)(context->data);
	result = (struct linked_list**)(context->next->data);
	if(!apply_visitor(matcher, data, context->next->next, node)) return 0;
	*result = node;
	return 1;
}
