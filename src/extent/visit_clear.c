/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

int visit_clear(struct extent *data, void *context, struct linked_list *node){
	(void)context;
	if(!data) return 1;
	if(!(data->bytes)) return 0;
	if(!(data->len)) return 0;
	memset(data->bytes, 0, data->len);
	data->len = 0;
	data->bytes = 0;
	node->data = 0;
	return 0;
}
