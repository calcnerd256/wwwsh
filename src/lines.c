/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <stdlib.h>
#include "./lines.h"

int init_ransom(struct linked_list *head){
	head->data = 0;
	head->next = 0;
	return 0;
}
int append_to_ransom(struct linked_list *ransom, char *bytes, size_t len){
	struct extent *suffix;
	int status;
	suffix = malloc(sizeof(struct extent));
	suffix->bytes = bytes;
	suffix->len = len;
	status = append(ransom, (void*)suffix);
	if(status){
		free(suffix);
		return 1;
	}
	return 0;
}
int visit_clean_extent(struct extent *ptr, void *context, struct linked_list *node){
	ptr->bytes = 0;
	ptr->len = 0;
	return (int)(0 * (long)context * (long)node);
}
int visit_clean_ransom(struct linked_list *ransom, void *context, struct linked_list *node){
	/* the first element is always a dummy */
	clean_and_free_list(ransom->next, (visitor_t)(&visit_clean_extent), context);
	ransom->next = 0;
	return (int)(0 * (long)node);
}
