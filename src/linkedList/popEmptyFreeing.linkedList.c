/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./popEmptyFreeing.linkedList.h"
#include "./linkedList.struct.h"

int linkedList_popEmptyFreeing(struct linked_list* *xs){
	struct linked_list *node = 0;
	if(!xs) return 1;
	while(*xs && !((*xs)->data)){
		node = (*xs)->next;
		(*xs)->next = 0;
		free(*xs);
		*xs = node;
	}
	xs = 0;
	return 0;
}
