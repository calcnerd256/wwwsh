/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "./process.h"
#include "./request.h"


int childProcessResource_spawn_child(int input, int output, char* cmd){
	int flags;
	close(0);
	close(1);
	close(2);
	flags = fcntl(input, F_GETFD);
	flags &= ~FD_CLOEXEC;
	fcntl(input, F_SETFD, flags);
	if(-1 == dup2(input, 0)) return 1;
	flags = fcntl(output, F_GETFD);
	flags &= ~FD_CLOEXEC;
	fcntl(output, F_SETFD, flags);
	if(-1 == dup2(output, 1)){
		close(0);
		return 1;
	}
	if(-1 == dup2(output, 2)){
		close(0);
		close(1);
		return 1;
	}
	if(execl("/bin/bash", "bash", "-c", cmd, (char*)0))
		return 1;
	return 1;
}
int childProcessResource_init_spawn(struct childProcessResource *child, struct extent* command){
	int ki[2];
	int ko[2];
	char cmd[1024];
	if(!child) return 1;
	if(!command) return 1;
	if(command->len > 1023) return 1;
	cmd[0] = 0;
	cmd[1023] = 0;
	cmd[command->len] = 0;
	strncpy(cmd, command->bytes, command->len);
	if(pipe2(ki, O_CLOEXEC)) return 1;
	child->input = ki[0];
	if(pipe2(ko, O_CLOEXEC)){
		close(ki[0]);
		close(ki[1]);
		child->input = -1;
		return 1;
	}
	child->output = ko[1];
	child->pid = fork();
	if(child->pid < 0){
		close(ki[0]);
		close(ki[1]);
		close(ko[0]);
		close(ko[1]);
		child->input = -1;
		child->output = -1;
		return 1;
	}
	if(!(child->pid)){
		close(ki[1]);
		close(ko[0]);
		if(childProcessResource_spawn_child(child->input, child->output, cmd)){
			close(child->input);
			child->input = -1;
			close(child->output);
			child->output = -1;
			_exit(1);
			return 1;
		}
	}
	close(child->input);
	child->input = ki[1];
	close(child->output);
	child->output = ko[0];
	init_pool(&(child->allocations));
	chunkStream_init(&(child->outputStream), &(child->allocations));
	child->url.len = 0;
	child->url.bytes = 0;
	return 0;
}

int childProcessResource_urlId(struct childProcessResource *kid, unsigned long int id){
	char idStr[256];
	int i;
	char hexit;
	char c;
	if(!kid) return 2;

	memset(idStr, 0, 256);
	i = 0;
	while(id){
		if(i >= 255) return 4;
		hexit = id % 16;
		id >>= 4;
		if(!hexit) c = '0';
		else{
			if(hexit < 10)
				c = '1' + hexit - 1;
			else
				c = 'a' + hexit - 10;
		}
		idStr[i++] = c;
	}
	kid->url.len = 9 + i;
	kid->url.bytes = palloc(&(kid->allocations), kid->url.len + 1);
	strncpy(kid->url.bytes, "/process/", 9);
	strncpy(kid->url.bytes + 9, idStr, i);
	kid->url.bytes[kid->url.len] = 0;
	return 0;
}

int childProcessResource_initResource(struct childProcessResource *kid){
	kid->node = malloc(sizeof(struct linked_list));
	kid->linkNode_resources = malloc(sizeof(struct linked_list));
	kid->resource.url.bytes = kid->url.bytes;
	kid->resource.url.len = kid->url.len;
	kid->node->data = kid;
	kid->node->next = 0;
	return 0;
}

int childProcessResource_initDeleteForm(struct childProcessResource *kid){
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

	return 0;
}


int childProcessResource_readChunkp(struct childProcessResource *kid){
	char buf[CHUNK_SIZE + 1];
	int chunkLen;
	if(!kid) return 0;
	if(-1 == kid->output) return 0;
	if(!fd_canReadp(kid->output)) return 0;
	buf[0] = 0;
	buf[CHUNK_SIZE] = 0;
	chunkLen = read(kid->output, buf, CHUNK_SIZE);
	if(-1 == chunkLen){
		close(kid->output);
		kid->output = -1;
		return 0;
	}
	buf[chunkLen] = 0;
	if(!chunkLen){
		close(kid->output);
		kid->output = -1;
		return 0;
	}
	chunkStream_append(&(kid->outputStream), buf, chunkLen);
	return 1;
}

int childProcessResource_close(struct childProcessResource *kid){
	if(-1 != kid->input)
		close(kid->input);
	kid->input = -1;
	if(-1 != kid->output)
		close(kid->output);
	kid->output = -1;
	if(-1 != kid->pid)
		if(kid->pid != waitpid(kid->pid, 0, WNOHANG)){
			/* TODO: kill -9 and then wait without wnohang */
		}
	kid->pid = -1;
	if(kid->node)
		kid->node->data = 0;
	kid->node = 0;
	return 0;
}



int childProcessResource_steppedp(struct childProcessResource *kid){
	if(!kid) return 0;
	if(-1 == kid->pid) return 0;
	if(kid->pid == waitpid(kid->pid, 0, WNOHANG)){
		kid->pid = -1;
		while(childProcessResource_readChunkp(kid));
		childProcessResource_close(kid);
		return 1;
	}
	return childProcessResource_readChunkp(kid);
}


int childProcessResource_urlMatchesp(struct httpResource *resource, struct extent*url){
	if(!resource) return 0;
	if(!url) return 0;
	if(!(resource->context)) return 0;
	return extent_url_equalsp(&(((struct childProcessResource*)(resource->context))->url), url);
}


int childProcessResource_canRespondp(struct httpResource *resource, struct incomingHttpRequest *request){
	(void)resource;
	(void)request;
	return 1;
}


int childProcessResource_remove(struct childProcessResource *kid){
	childProcessResource_close(kid);
	kid->linkNode_resources->data = 0;
	kid->linkNode_resources_deleteForm->data = 0;
	free_pool(&(kid->allocations));
	memset(kid, 0, sizeof(struct childProcessResource));
	free(kid);
	return 0;
}

int childProcessResource_streamOutput(struct childProcessResource *kid, struct incomingHttpRequest *request, struct extent *contentType){
	struct extent reason;
	struct extent contentType_key;
	struct linked_list extrahead_node;
	struct linked_list extrahead_key;
	struct linked_list extrahead_val;
	struct linked_list *cursor;
	if(!kid) return 1;
	if(!request) return 1;
	if(!contentType) return 1;
	cursor = kid->outputStream.chunk_list.head;
	if(point_extent_at_nice_string(&reason, "OK")) return 2;
	if(point_extent_at_nice_string(&contentType_key, "Content-Type")) return 2;
	extrahead_node.data = &extrahead_key;
	extrahead_node.next = 0;
	extrahead_key.data = &contentType_key;
	extrahead_key.next = &extrahead_val;
	extrahead_val.data = contentType;
	extrahead_val.next = 0;
	if(incomingHttpRequest_beginChunkedResponse(request, 200, &reason, &extrahead_node)) return 4;
	while(cursor){
		incomingHttpRequest_write_chunk(request, ((struct extent*)(cursor->data))->bytes, ((struct extent*)(cursor->data))->len);
		cursor = cursor->next;
	}
	return incomingHttpRequest_sendLastChunk(request, 0);
}
int childProcessResource_respond_output(struct httpResource *resource, struct incomingHttpRequest *request){
	struct extent textPlain;
	if(!resource) return 1;
	if(!request) return 1;
	if(point_extent_at_nice_string(&textPlain, "text/plain")) return 3;
	return childProcessResource_streamOutput((struct childProcessResource*)(resource->context), request, &textPlain);
}

int childProcessResource_respond_remove(struct httpResource *resource, struct incomingHttpRequest *request){
	if(!resource) return 1;
	if(!request) return 1;
	childProcessResource_remove((struct childProcessResource*)(resource->context));
	if(incomingHttpRequest_beginChunkedHtmlBody(request, 0, "deleted", 7)) return 2;
	incomingHttpRequest_writelnChunk_niceString(request, "  Process successfully removed, probably.");
	return incomingHttpRequest_endChunkedHtmlBody(request, 0);
}

int childProcessResource_respond(struct httpResource *resource, struct incomingHttpRequest *request){
	if(!resource) return 1;
	if(!request) return 1;
	return childProcessResource_respond_output(resource, request);
}


int childProcessResource_deleteForm_respond_POST(struct httpResource *resource, struct incomingHttpRequest *request, struct linked_list *formdata){
	struct extent *fieldName;
	if(!resource) return 1;
	if(!request) return 1;
	while(formdata){
		fieldName = (struct extent*)(((struct linked_list*)(formdata->data))->data);
		if(7 == fieldName->len)
			if(fieldName->bytes)
				if(!strncmp(fieldName->bytes, "confirm", 7))
					return childProcessResource_respond_remove(resource, request);
		formdata = formdata->next;
	}
	return childProcessResource_respond_output(resource, request);
}
int childProcessResource_deleteForm_respond(struct httpResource *resource, struct incomingHttpRequest *request){
	struct childProcessResource *kid;
	struct form *form;
	if(!resource) return 1;
	if(!request) return 1;
	if(!(request->input.method)) return 2;
	if(3 > request->input.method->len)
		return incomingHttpRequest_respond_badMethod(request);
	if(!(resource->context)) return 2;
	kid = resource->context;
	form = &(kid->deleteForm);
	if(!strncmp("GET", request->input.method->bytes, 4))
		return form_respond_GET(form, request, &(resource->url));
	if(4 > request->input.method->len)
		return incomingHttpRequest_respond_badMethod(request);
	if(strncmp("POST", request->input.method->bytes, 5))
		return incomingHttpRequest_respond_badMethod(request);
	return staticFormResource_respond_POST(resource, request, form);
}
