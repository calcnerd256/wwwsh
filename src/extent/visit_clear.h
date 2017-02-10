/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#ifndef INCLUDE_visitClear
# define INCLUDE_visitClear

# include "./extent.struct.h"
# include "./linkedList.struct.h"

int visit_clear(
	struct extent *data,
	void *context,
	struct linked_list *node
);

#endif
