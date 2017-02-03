/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./append.dequoid.h"
#include "./dequoid.struct.h"
#include "./linkedList.struct.h"

int dequoid_append(struct dequoid *list, void *data, struct linked_list *node){
	node->next = 0;
	node->data = data;
	if(!(list->tail)) list->tail = list->head;
	if(!(list->tail)){
		list->head = node;
		list->tail = list->head;
	}
	else
		list->tail->next = node;
	list->tail = node;
	return 0;
}
