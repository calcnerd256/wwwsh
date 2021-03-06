/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./visit_extent_sumLength.h"
#include "./extent.struct.h"
#include "./size_t.typedef.h"
#include "./linkedList.struct.h"

int visit_extent_sumLength(struct extent *chunk, size_t *len, struct linked_list *node){
	if(!node) return 1;
	node = 0;
	if(!chunk) return 1;
	if(!len) return 1;
	*len += chunk->len;
	chunk = 0;
	len = 0;
	return 0;
}
