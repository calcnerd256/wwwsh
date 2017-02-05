char *matcher;
struct linked_list* *result;
matcher = (char*)(context->data);
result = (struct linked_list**)(context->next->data);
if(!apply_visitor(matcher, data, context->next->next, node)) return 0;
*result = node;
return 1;
