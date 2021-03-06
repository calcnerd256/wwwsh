/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#ifndef INCLUDE_visit_extent_sumLength
# define INCLUDE_visit_extent_sumLength

# include "./extent.struct.h"
# include "./size_t.typedef.h"
# include "./linkedList.struct.h"

int visit_extent_sumLength(
	struct extent *chunk,
	size_t *len,
	struct linked_list *node
);

#endif
