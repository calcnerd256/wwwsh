/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#ifndef INCLUDE_dequoid_STRUCT
#define INCLUDE_dequoid_STRUCT

#include "./linkedList.struct.h"

struct dequoid{
	struct linked_list *head;
  	struct linked_list *tail;
};

#endif
