/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./first_matching.h"
#include "./linkedList.struct.h"
#include "./visitor_t.typedef.h"
#include "./alloc_copy_visitor.h"
#include "./traverse_linked_list.h"
#include "./free_visitor_copy.h"

int first_matching(struct linked_list *head, visitor_t matcher, struct linked_list *context, struct linked_list* *out_match){
	struct linked_list outer_context[3];
	int result;
	outer_context[0].data = alloc_copy_visitor(matcher);
	outer_context[0].next = &(outer_context[1]);
	outer_context[1].data = (void*)out_match;
	outer_context[1].next = context;
	result = !(traverse_linked_list(head, (visitor_t)(&visit_matcher), outer_context));
	free_visitor_copy(outer_context[0].data);
	return result;
}
