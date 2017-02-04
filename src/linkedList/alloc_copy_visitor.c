/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./alloc_copy_visitor.h"
#include "./visitor_t.typedef.h"
#include "./malloc.h"
#include "./memcpy.h"

char *alloc_copy_visitor(visitor_t visitor){
	char *result;
	result = malloc(sizeof(visitor_t));
	memcpy(result, &visitor, sizeof(visitor_t));
	return result;
}
