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

size_t ransom_len(struct linked_list *ransom);
char *flatten_ransom(struct linked_list *ransom);

int visit_print_ransom(struct linked_list *head, void *context, struct linked_list *node);

int split_extent_on_byte(struct extent *inp, char b, struct extent *l, struct extent *r);
