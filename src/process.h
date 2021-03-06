/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./form.h"

struct childProcessResource{
	struct mempool allocations;
	struct chunkStream outputStream;
	struct httpResource resource;
	struct extent url;
	struct form deleteForm;
	struct httpResource deleteForm_resource;
	struct extent deleteForm_title;
	struct linked_list deleteForm_confirmFieldNode;
	struct linked_list confirmField_node_name;
	struct linked_list confirmField_node_tag;
	struct linked_list confirmField_node_type;
	struct extent confirmFieldName;
	struct extent confirmFieldTag;
	struct extent confirmFieldType;
	struct linked_list *node;
	struct linked_list *linkNode_resources;
	struct linked_list *linkNode_resources_deleteForm;
	pid_t pid;
	int input;
	int output;
};


int childProcessResource_init_spawn(struct childProcessResource *child, struct extent* command);

int childProcessResource_urlId(struct childProcessResource *kid, unsigned long int id);
int childProcessResource_initResource(struct childProcessResource *kid);
int childProcessResource_initDeleteForm(struct childProcessResource *kid);


int childProcessResource_steppedp(struct childProcessResource *kid);

int childProcessResource_urlMatchesp(struct httpResource *resource, struct extent*url);
int childProcessResource_canRespondp(struct httpResource *resource, struct incomingHttpRequest *request);
int childProcessResource_respond(struct httpResource *resource, struct incomingHttpRequest *request);


int childProcessResource_deleteForm_respond_POST(struct httpResource *resource, struct incomingHttpRequest *request, struct linked_list *formdata);
int childProcessResource_deleteForm_respond(struct httpResource *resource, struct incomingHttpRequest *request);
