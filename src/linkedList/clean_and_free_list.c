/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./clean_and_free_list.h"
#include "./linkedList.struct.h"
#include "./visitor_t.typedef.h"
#include "./traverse_linked_list.h"
#include "./free.h"

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
