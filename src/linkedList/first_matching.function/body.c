struct linked_list outer_context[3];
int result;
outer_context[0].data = alloc_copy_visitor(matcher);
outer_context[0].next = &(outer_context[1]);
outer_context[1].data = (void*)out_match;
outer_context[1].next = context;
result = !(traverse_linked_list(head, (visitor_t)(&visit_matcher), outer_context));
free_visitor_copy(outer_context[0].data);
return result;
