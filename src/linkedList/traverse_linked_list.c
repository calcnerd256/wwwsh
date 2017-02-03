/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./traverse_linked_list.h"
#include "./linkedList.struct.h"
#include "./visitor_t.typedef.h"

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
