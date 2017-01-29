/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "./server.h"
#include "./network.h"
#include "./request.h"


int httpServer_init(struct httpServer *server){
	server->listeningSocket_fileDescriptor = -1;
	server->resources = 0;
	server->children = 0;
	server->nextChild = 1;
	server = 0;
	return 0;
}

int httpServer_pushResource(struct httpServer *server, struct linked_list *new_head, struct httpResource *resource, int (*urlMatchesp)(struct httpResource*, struct extent*), int (*canRespondp)(struct httpResource*, struct incomingHttpRequest*), int (*respond)(struct httpResource*, struct incomingHttpRequest*), void *context){
	if(!server) return 1;
	if(!new_head) return 2;
	if(!respond) return 2;
	resource->urlMatchesp = urlMatchesp;
	resource->canRespondp = canRespondp;
	resource->respond = respond;
	resource->context = context;
	new_head->next = server->resources;
	new_head->data = resource;
	server->resources = new_head;
	return 0;
}

int httpServer_pushChildProcess(struct httpServer *server, struct childProcessResource *kid){
	char childId[256];
	unsigned long int lid;
	int i;
	char c;
	char hexit;
	if(!server) return 1;
	if(!kid) return 2;

	memset(childId, 0, 256);
	lid = server->nextChild++;
	i = 0;
	while(lid){
		if(i >= 255) return 4;
		hexit = lid % 16;
		lid >>= 4;
		if(!hexit) c = '0';
		else{
			if(hexit < 10)
				c = '1' + hexit - 1;
			else
				c = 'a' + hexit - 10;
		}
		childId[i++] = c;
	}

	kid->node = malloc(sizeof(struct linked_list));
	kid->linkNode_resources = malloc(sizeof(struct linked_list));
	kid->url.len = 9 + i;
	kid->url.bytes = palloc(&(kid->allocations), kid->url.len + 1);
	strncpy(kid->url.bytes, "/process/", 9);
	strncpy(kid->url.bytes + 9, childId, i);
	kid->url.bytes[kid->url.len] = 0;
	kid->resource.url.bytes = kid->url.bytes;
	kid->resource.url.len = kid->url.len;

	kid->node->next = server->children;
	kid->node->data = kid;
	server->children = kid->node;

	kid->linkNode_resources_deleteForm = malloc(sizeof(struct linked_list));
	point_extent_at_nice_string(&(kid->deleteForm_title), "delete process");
	point_extent_at_nice_string(&(kid->confirmFieldName), "confirm");
	point_extent_at_nice_string(&(kid->confirmFieldTag), "input");
	point_extent_at_nice_string(&(kid->confirmFieldType), "checkbox");
	kid->confirmField_node_type.data = &(kid->confirmFieldType);
	kid->confirmField_node_type.next = 0;
	kid->confirmField_node_tag.data = &(kid->confirmFieldTag);
	kid->confirmField_node_tag.next = &(kid->confirmField_node_type);
	kid->confirmField_node_name.data = &(kid->confirmFieldName);
	kid->confirmField_node_name.next = &(kid->confirmField_node_tag);
	kid->deleteForm_confirmFieldNode.data = &(kid->confirmField_node_name);
	kid->deleteForm_confirmFieldNode.next = 0;
	kid->deleteForm.title = &(kid->deleteForm_title);
	kid->deleteForm.fields = &(kid->deleteForm_confirmFieldNode);
	kid->deleteForm.action = &(kid->deleteForm_resource);
	kid->deleteForm.respond_POST = &childProcessResource_deleteForm_respond_POST;
	kid->deleteForm_resource.url.bytes = palloc(&(kid->allocations), kid->url.len + 8 + 1);
	kid->deleteForm_resource.url.len = kid->url.len + 8;
	strncpy(kid->deleteForm_resource.url.bytes, kid->url.bytes, kid->url.len);
	strncpy(kid->deleteForm_resource.url.bytes + kid->url.len, "/delete/", 9);


	if(httpServer_pushResource(server, kid->linkNode_resources, &(kid->resource), &childProcessResource_urlMatchesp, &childProcessResource_canRespondp, &childProcessResource_respond, kid))
		return 4;

	if(httpServer_pushResource(server, kid->linkNode_resources_deleteForm, &(kid->deleteForm_resource), 0, &staticFormResource_canRespondp, &childProcessResource_deleteForm_respond, kid))
		return 5;

	return 0;
}

int httpServer_listen(struct httpServer *server, char* port_number, int backlog){
	int sockfd = -1;
	if(get_socket(port_number, &sockfd)){
		server = 0;
		port_number = 0;
		backlog = 0;
		sockfd = 0;
		return 1;
	}
	port_number = 0;
	server->listeningSocket_fileDescriptor = sockfd;
	server = 0;
	if(listen(sockfd, backlog)){
		backlog = 0;
		sockfd = 0;
		return 2;
	}
	sockfd = 0;
	backlog = 0;
	return 0;
}

/* TODO: consider cleaning up all open connections when closing the server, somehow */
int httpServer_close(struct httpServer *server){
	server->resources = 0;
	if(-1 == server->listeningSocket_fileDescriptor){
		server = 0;
		return 0;
	}
	if(shutdown(server->listeningSocket_fileDescriptor, SHUT_RDWR)){
		server = 0;
		return 1;
	}
	if(close(server->listeningSocket_fileDescriptor)){
		server = 0;
		return 2;
	}
	server->listeningSocket_fileDescriptor = -1;
	server = 0;
	return 0;
}

int httpServer_acceptNewConnection_fd(int fd){
	struct sockaddr address;
	socklen_t length;
	int result;

	memset(&address, 0, sizeof(struct sockaddr));
	length = 0;
	result = accept4(fd, &address, &length, SOCK_CLOEXEC);
	fd = 0;
	memset(&address, 0, sizeof(struct sockaddr));
	length = 0;
	return result;
}

int match_httpResource_url(struct httpResource *resource, struct extent *url, struct linked_list *node){
	(void)node;
	node = 0;
	if(!url) return 0;
	if(!resource) return 0;
	if(resource->url.bytes)
		if(resource->url.len)
			if(extent_url_equalsp(&(resource->url), url))
				return 1;
	if(!(resource->urlMatchesp)) return 0;
	return (*(resource->urlMatchesp))(resource, url);
}
struct httpResource* httpServer_locateResource(struct httpServer *server, struct extent *url){
	struct httpResource *retval = 0;
	struct linked_list *match_node = 0;
	struct linked_list *head = server->resources;
	visitor_t matcher = (visitor_t)(&match_httpResource_url);
	int result = first_matching(head, matcher, (struct linked_list*)url, &match_node);
	server = 0;
	url = 0;
	head = 0;
	matcher = 0;
	if(result){
		match_node = 0;
		result = 0;
		return 0;
	}
	result = 0;
	if(!match_node){
		match_node = 0;
		return 0;
	}
	retval = match_node->data;
	match_node = 0;
	return retval;
}


int visit_incomingHttpRequest_processStep(struct incomingHttpRequest *conn, int *context, struct linked_list *node){
	(void)node;
	if(incomingHttpRequest_processSteppedp(conn)) *context = 1;
	conn = 0;
	context = 0;
	node = 0;
	return 0;
}
int visit_childProcessResource_processStep(struct childProcessResource *kid, int *context, struct linked_list *node){
	(void)node;
	if(childProcessResource_steppedp(kid)) *context = 1;
	kid = 0;
	context = 0;
	node = 0;
	return 0;
}
