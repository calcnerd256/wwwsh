/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./removeMiddleEmptiesFreeing.linkedList.h"
#include "./linkedList.struct.h"

#include "./stdlib.inc.h"

int linkedList_removeMiddleEmptiesFreeing(struct linked_list *xs){
	struct linked_list *node = xs;
	struct linked_list *middle;
	while(node){
		while(node->next && !(node->next->data)){
			middle = node->next;
			node->next = middle->next;
			middle->next = 0;
			free(middle);
		}
		node = node->next;
	}
	xs = 0;
	node = 0;
	middle = 0;
	return 0;
}
