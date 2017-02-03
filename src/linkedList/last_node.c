/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./last_node.h"
#include "./linkedList.struct.h"
#include "./first_matching.h"

struct linked_list *last_node(struct linked_list *head){
	struct linked_list *result = 0;
	if(first_matching(head, match_last, 0, &result)) return 0;
	return result;
}
