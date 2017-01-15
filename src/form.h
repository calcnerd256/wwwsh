/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <stdio.h>
#include "./server_structs.h"

struct form{
	struct extent *title;
	struct linked_list *fields;
	struct httpResource *action;
	int (*respond_POST)(struct httpResource*, struct incomingHttpRequest*, struct linked_list*);
};
struct staticFormResource{
	struct linked_list node;
	struct httpResource resource;
	struct extent title;
	struct extent url;
	struct form *form;
	void *context;
};

int staticFormResource_canRespondp(struct httpResource *res, struct incomingHttpRequest *req);


int form_respond_GET(struct form *form, struct incomingHttpRequest *req, struct extent *action);
int staticFormResource_respond_GET(struct httpResource *res, struct incomingHttpRequest *req, struct form *form);
int staticFormResource_respond_POST(struct httpResource *res, struct incomingHttpRequest *req, struct form *form);

int staticFormResource_init(
	struct staticFormResource *resource,
	struct httpServer *server,
	struct form *form,
	char* url,
	char* title,
	struct linked_list *fields,
	int (*respond_POST)(struct httpResource*, struct incomingHttpRequest*, struct linked_list*),
	void *context
);
