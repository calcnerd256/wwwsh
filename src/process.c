/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include "./process.h"
#include "./request.h"

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
	childProcessResource_respond_output(resource, request);
	return childProcessResource_remove((struct childProcessResource*)(resource->context));
	return childProcessResource_respond_remove(resource, request);
}
