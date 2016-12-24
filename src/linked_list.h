/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#ifndef INCLUDE_LINKED_LIST
#define INCLUDE_LINKED_LIST

struct linked_list{
	void *data;
	struct linked_list *next;
};

typedef int (*visitor_t)(void*, void*, struct linked_list*);

char *alloc_copy_visitor(visitor_t visitor);
int free_visitor_copy(char *bytes);
int apply_visitor(char *fnptr_bytes, void* data, void* context, struct linked_list *node);
int traverse_linked_list(struct linked_list *head, visitor_t visitor, void *context);
int clean_and_free_list(struct linked_list *head, visitor_t cleaner, void *context);
int visit_matcher(void *data, struct linked_list *context, struct linked_list *node);
int first_matching(struct linked_list *head, visitor_t matcher, struct linked_list *context, struct linked_list* *out_match);
int match_last(void *data, void *context, struct linked_list *node);
struct linked_list *last_node(struct linked_list *head);
int append(struct linked_list *head, void* data);

#endif
