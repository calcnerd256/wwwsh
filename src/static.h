/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./server_structs.h"

struct staticGetResource{
	struct extent *url;
	struct extent *body;
	struct linked_list *headers;
};

int match_resource_url(struct staticGetResource *resource, struct extent *url, struct linked_list *node);
int staticGetResource_urlMatchesp(struct httpResource *resource, struct extent *url);
int staticGetResource_respond(struct httpResource *resource, struct conn_bundle *connection);

struct contiguousHtmlResource{
	struct linked_list link_node;
	struct httpResource resource;
	struct staticGetResource staticResource;
	struct extent url;
	struct extent body;
	struct linked_list headerTop;
	struct linked_list keyNode;
	struct linked_list valNode;
	struct extent key;
	struct extent val;
};

int contiguousHtmlResource_init(struct contiguousHtmlResource *res, char *url, char *body);
