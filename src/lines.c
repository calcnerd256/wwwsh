/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
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


int visit_extent_sum_len(struct extent *ext, size_t *result, void *tail){
	*result += ext->len;
	(void)tail;
	return 0;
}
size_t ransom_len(struct linked_list *ransom){
	size_t result = 0;
	if(!ransom) return -1;
	if(traverse_linked_list(ransom->next, (visitor_t)visit_extent_sum_len, &result))
		return -1;
	return result;
}

int visit_extent_append_string(struct extent *ext, struct linked_list *context, void *tail){
	char *buf;
	ptrdiff_t *offset;
	buf = (char*)context->data;
	offset = (ptrdiff_t*)context->next->data;
	memcpy(buf + *offset, ext->bytes, ext->len);
	*offset += ext->len;
	(void)tail;
	return 0;
}
char *flatten_ransom(struct linked_list *ransom){
	size_t len = ransom_len(ransom);
	char *result = malloc(len + 1);
	struct linked_list context;
	struct linked_list tail;
	ptrdiff_t offset = 0;
	tail.data = &offset;
	tail.next = 0;
	context.next = &tail;
	context.data = result;
  if(traverse_linked_list(ransom->next, (visitor_t)visit_extent_append_string, &context)){
		free(result);
		return 0;
	}
  result[len] = 0;
	return result;
}

int visit_print_extent(struct extent *clip, void *context, struct linked_list *node){
	size_t i;
	if(!clip) return 1;
	for(i = 0; i < clip->len; i++)
		printf(context ? "%02X" : "%c", clip->bytes[i]);
	if(context)
		printf(" ");
	return (int)(0 * (long)node);
}
int visit_print_ransom(struct linked_list *head, void *context, struct linked_list *node){
	int status = 0;
	(void)node;
	printf("(%d bytes) ", (int)ransom_len(head));
	if(!head) return 1;
	status = traverse_linked_list(head->next, (visitor_t)&visit_print_extent, context);
	if(context)
		printf("\n");
	return status;
}

int split_extent_on_byte(struct extent *inp, char b, struct extent *l, struct extent *r){
	size_t i;
	for(i = 0; i < inp->len; i++)
		if(b == inp->bytes[i]){
			l->bytes = inp->bytes;
			l->len = i - 1;
			r->bytes = inp->bytes + i + 1;
			r->len = inp->len - i - 1;
			return 0;
		}
	l->bytes = inp->bytes;
	l->len = inp->len;
	r->bytes = inp->bytes + inp->len;
	r->len = 0;
	return 1;
}

/*
int copy_ransom(struct linked_list *ransom, struct linked_list *out, struct mempool *pool){
}
*/

/*
int split_ransom_on_byte(struct linked_list *ransom, char b, struct linked_list *l, struct linked_list *r){
}
*/
