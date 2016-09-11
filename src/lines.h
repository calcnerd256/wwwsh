/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <sys/types.h>
#include "./linked_list.h"

struct extent{
	char *bytes;
	size_t len;
};

int init_ransom(struct linked_list *head);
int append_to_ransom(struct linked_list *ransom, char *bytes, size_t len);
int visit_clean_extent(struct extent *ptr, void *context, struct linked_list *node);
int visit_clean_ransom(struct linked_list *ransom, void *context, struct linked_list *node);
