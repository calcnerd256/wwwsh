/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./free_visitor_copy.h"
#include "./visitor_t.typedef.h"

#ifndef INCLUDE_string
# include <string.h>
# define INCLUDE_string
#endif

#ifndef INCLUDE_stdlib
# include <stdlib.h>
# define INCLUDE_stdlib
#endif

int free_visitor_copy(char *bytes){
	memset(bytes, 0, sizeof(visitor_t));
	free(bytes);
	return 0;
}
