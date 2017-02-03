/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./init.dequoid.h"
#include "./dequoid.struct.h"

int dequoid_init(struct dequoid *list){
	list->head = 0;
	list->tail = 0;
	return 0;
}
