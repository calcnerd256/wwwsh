/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#ifndef INCLUDE_dequoid_append
# define INCLUDE_dequoid_append

# include "./dequoid.struct.h"
# include "./linkedList.struct.h"

int dequoid_append(
	struct dequoid *list,
	void *data,
	struct linked_list *node
);

#endif
