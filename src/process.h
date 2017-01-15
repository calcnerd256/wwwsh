/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./server_structs.h"

struct childProcessResource{
	struct mempool allocations;
	struct chunkStream outputStream;
	struct httpResource resource;
	struct extent url;
	struct linked_list *node;
	struct linked_list *linkNode_resources;
	pid_t pid;
	int input;
	int output;
};

int childProcessResource_steppedp(struct childProcessResource *kid);

int childProcessResource_urlMatchesp(struct httpResource *resource, struct extent*url);
int childProcessResource_canRespondp(struct httpResource *resource, struct incomingHttpRequest *request);
int childProcessResource_respond(struct httpResource *resource, struct incomingHttpRequest *request);
