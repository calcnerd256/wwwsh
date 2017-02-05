/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./append.h"
#include "./linkedList.struct.h"
#include "./last_node.h"
#include "./malloc.h"

int append(struct linked_list *head, void* data){
	head = last_node(head);
	if(!head) return 1;
	head->next = malloc(sizeof(struct linked_list));
	head = head->next;
	head->data = data;
	head->next = 0;
	return 0;
}
