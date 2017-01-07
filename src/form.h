/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <stdio.h>
#include "./server_structs.h"

struct form{
	struct extent *title;
	struct linked_list *fields;
	struct httpResource *action;
	int (*respond_POST)(struct httpResource*, struct incomingHttpRequest*);
};
struct staticFormResource{
	struct linked_list node;
	struct httpResource resource;
	struct extent title;
	struct extent url;
	struct form *form;
	void *context;
};

int staticFormResource_init(struct staticFormResource *resource, struct form *form, char* url, char* title);

int staticFormResource_urlMatchesp(struct httpResource *res, struct extent *url);
int staticFormResource_canRespondp(struct httpResource *res, struct incomingHttpRequest *req);
int staticFormResource_respond_GET(struct httpResource *res, struct incomingHttpRequest *req);

int staticFormResource_respond(struct httpResource *res, struct incomingHttpRequest *req);
