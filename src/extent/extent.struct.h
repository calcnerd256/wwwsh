/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#ifndef INCLUDE_extent_STRUCT
# define INCLUDE_extent_STRUCT

# include "./size_t.typedef.h"

struct extent{
	char *bytes;
	size_t len;
};

#endif
