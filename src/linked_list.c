/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <stdlib.h>
#include <string.h>
#include "./linked_list.h"

char *alloc_copy_visitor(visitor_t visitor){
	char *result;
	result = malloc(sizeof(visitor_t));
	memcpy(result, &visitor, sizeof(visitor_t));
	return result;
}

int free_visitor_copy(char *bytes){
	memset(bytes, 0, sizeof(visitor_t));
	free(bytes);
	return 0;
}

int apply_visitor(char *fnptr_bytes, void* data, void* context, struct linked_list *node){
	visitor_t visitor;
	memcpy(&visitor, fnptr_bytes, sizeof(visitor_t));
	return (*visitor)(data, context, node);
}


int traverse_linked_list(struct linked_list *head, visitor_t visitor, void *context){
	int status;
	struct linked_list *cycle_detector;
	char counter;
	if(!head) return 0;
	cycle_detector = 0;
	counter = 0;
	while(head){
		if(cycle_detector == head) return 0;
		status = (*visitor)(head->data, context, head);
		if(status) return status;
		if(!cycle_detector) cycle_detector = head;
		head = head->next;
		if(counter) cycle_detector = cycle_detector->next;
		counter = !counter;
	}
	return 0;
}
int clean_and_free_list(struct linked_list *head, visitor_t cleaner, void *context){
	int status;
	struct linked_list *ptr;
	status = traverse_linked_list(head, cleaner, context);
	if(status) return status;
	while(head){
		head->data = 0;
		ptr = head->next;
		head->next = 0;
		free(head);
		head = ptr;
	}
	return 0;
}
int visit_matcher(void *data, struct linked_list *context, struct linked_list *node){
	char *matcher;
	struct linked_list* *result;
	matcher = (char*)(context->data);
	result = (struct linked_list**)(context->next->data);
	if(!apply_visitor(matcher, data, context->next->next, node)) return 0;
	*result = node;
	return 1;
}
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
int match_last(void *data, void *context, struct linked_list *node){
	(void)data;
	(void)context;
	return !(node->next);
}
struct linked_list *last_node(struct linked_list *head){
	struct linked_list *result = 0;
	if(first_matching(head, match_last, 0, &result)) return 0;
	return result;
}
int append(struct linked_list *head, void* data){
	head = last_node(head);
	if(!head) return 1;
	head->next = malloc(sizeof(struct linked_list));
	head = head->next;
	head->data = data;
	head->next = 0;
	return 0;
}

int visit_accumulate_length(void *data, size_t *context, struct linked_list *node){
	*context++ ? (void)data : (void)node;
	return 0;
}
size_t list_length(struct linked_list *head){
	size_t result = 0;
	if(traverse_linked_list(head, (visitor_t)visit_accumulate_length, &result))
		return -1;
	return result;
}
