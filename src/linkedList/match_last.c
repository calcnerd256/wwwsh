/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./match_last.h"
#include "./linkedList.struct.h"

int match_last(void *data, void *context, struct linked_list *node){
	(void)data;
	(void)context;
	return !(node->next);
}
