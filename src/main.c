/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include "./server.h"
#include "./request.h"
#include "./static.h"
#include "./form.h"


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

int childProcessResource_init_spawn(struct childProcessResource *child, char* cmd){
	int ki[2];
	int ko[2];
	if(!child) return 1;
	if(!cmd) return 1;
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

int childProcessResource_remove(struct childProcessResource *kid){
	childProcessResource_close(kid);
	kid->linkNode_resources->data = 0;
	free_pool(&(kid->allocations));
	memset(kid, 0, sizeof(struct childProcessResource));
	free(kid);
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

int childProcessResource_steppedp(struct childProcessResource *kid){
	if(!kid) return 0;
	if(-1 == kid->pid) return 0;
	if(kid->pid == waitpid(kid->pid, 0, WNOHANG)){
		printf("dying %d\n", kid->pid);
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
	if(incomingHttpRequest_beginChunkedHtmlOk(request, 0)) return 2;
	incomingHttpRequest_writelnChunk_niceString(request, "<html>");
	incomingHttpRequest_writelnChunk_niceString(request, " <head>");
	incomingHttpRequest_writelnChunk_niceString(request, "  <title>deleted</title>");
	incomingHttpRequest_writelnChunk_niceString(request, " </head>");
	incomingHttpRequest_writelnChunk_niceString(request, " <body>");
	incomingHttpRequest_writelnChunk_niceString(request, "  Process successfully removed, probably.");
	incomingHttpRequest_writelnChunk_niceString(request, " </body>");
	incomingHttpRequest_writelnChunk_niceString(request, "</html>");
	return incomingHttpRequest_sendLastChunk(request, 0);
}
int childProcessResource_respond(struct httpResource *resource, struct incomingHttpRequest *request){
	if(!resource) return 1;
	if(!request) return 1;
	return childProcessResource_respond_output(resource, request);
	return childProcessResource_respond_remove(resource, request);
}

int spawnForm_respond_POST(struct httpResource *res, struct incomingHttpRequest *req, struct linked_list *formData){
	struct childProcessResource *child;
	struct httpServer *server;
	struct staticFormResource *fr;
	char pid[256];
	int plen;
	if(!res) return 1;
	if(!req) return 1;
	if(!formData) return 1;
	fr = res->context;
	if(!fr) return 1;
	server = ((struct linked_list*)(fr->context))->data;
	if(!server) return 1;
	child = malloc(sizeof(struct childProcessResource));
	if(childProcessResource_init_spawn(child, "ls -al")) return 2;
	if(httpServer_pushChildProcess(server, child)) return 3;
	memset(pid, 0, 256);
	plen = snprintf(pid, 256, "%d", child->pid);
	pid[255] = 0;
	if(-1 == plen) return 8;
	if(255 < plen) return 9;
	pid[plen] = 0;

	if(incomingHttpRequest_beginChunkedHtmlOk(req, 0))
		return 4;
	if(incomingHttpRequest_writelnChunk_niceString(req, "<html>"))
		return 5;
	if(incomingHttpRequest_writelnChunk_niceString(req, " <head>"))
		return 5;
	if(incomingHttpRequest_writelnChunk_niceString(req, "  <title>spawned a process</title>"))
		return 5;
	if(incomingHttpRequest_writelnChunk_niceString(req, " </head>"))
		return 5;
	if(incomingHttpRequest_writelnChunk_niceString(req, " <body>"))
		return 5;
	if(incomingHttpRequest_writeChunk_niceString(req, "  <a href=\""))
		return 5;
	if(incomingHttpRequest_writeChunk_htmlSafeExtent(req, &(child->url)))
		return 6;
	if(incomingHttpRequest_writelnChunk_niceString(req, "/\">spawned</a>"))
		return 7;
	if(incomingHttpRequest_writelnChunk_niceString(req, " </body>"))
		return 7;
	if(incomingHttpRequest_writelnChunk_niceString(req, "</html>"))
		return 7;
	return incomingHttpRequest_sendLastChunk(req, 0);
}

/*
	TODO
	move childProcessResource class into its own compilation unit
	make a resource that lists each childProcessResource or just lists all resources by URL

*/

int main(int argument_count, char* *arguments_vector){
	struct httpServer server;
	int status = 0;

	struct contiguousHtmlResource rootResourceStorage;

	struct staticFormResource formResource;
	struct mempool formAllocations;
	struct linked_list formContext;
	struct linked_list formPoolCell;
	struct linked_list fieldHead;
	struct linked_list fieldNameNode;
	struct linked_list fieldTagNode;
	struct extent fieldName;
	struct extent fieldTag;
	struct form formForm;

	if(2 > argument_count) return 1;
	if(3 < argument_count) return 1;
	if(2 < argument_count)
		if(!atoi(arguments_vector[2]))
			return 12;

	if(point_extent_at_nice_string(&fieldName, "cmd")) return 11;
	if(point_extent_at_nice_string(&fieldTag, "textarea")) return 11;
	if(init_pool(&formAllocations)) return 8;
	if(httpServer_init(&server)) return 2;

	status = contiguousHtmlResource_init(
		&rootResourceStorage,
		"/",
		"<html>\r\n"
		" <head>\r\n"
		"  <title>Hello World!</title>\r\n"
		" </head>\r\n"
		" <body>\r\n"
		"  <h1>Hello, World!</h1>\r\n"
		"  <p>\r\n"
		"   This webserver is written in C.\r\n"
		"   I'm pretty proud of it!\r\n"
		"  </p>\r\n"
		"  <p>\r\n"
		"   Coming soon:\r\n"
		"   <a href=\"./spawn/\">a form</a>\r\n"
		"  </p>\r\n"
		" </body>\r\n"
		"</html>\r\n"
	);
	if(status) return 3;
	status = httpServer_pushResource(
		&server,
		&(rootResourceStorage.link_node),
		&(rootResourceStorage.resource),
		&staticGetResource_urlMatchesp,
		&staticGetResource_canRespondp,
		&staticGetResource_respond,
		&(rootResourceStorage.staticResource)
	);
	if(status) return 4;
	/*
		You MUST NOT set rootResourceStorage.link_node.data to 0,
		or else the server will try to free stacked memory.
		If you wish to remove this cell from the linked list,
		then you must remove it by pointing its predecessor at its successor.
	*/

	formContext.data = &server;
	formContext.next = &formPoolCell;
	formPoolCell.data = &formAllocations;
	formPoolCell.next = 0;

	fieldHead.data = &fieldNameNode;
	fieldHead.next = 0;
	fieldNameNode.data = &fieldName;
	fieldNameNode.next = &fieldTagNode;
	fieldTagNode.data = &fieldTag;
	fieldTagNode.next = 0;

	status = staticFormResource_init(
		&formResource,
		&server,
		&formForm,
		"/spawn/",
		"spawn",
		&fieldHead,
		&spawnForm_respond_POST,
		&formContext
	);
	if(status)
		return 7;
	/*
		You MUST NOT set formResource.node.data to 0,
		or else the server will try to free stacked memory.
		If you wish to remove this cell from the linked list,
		then you must remove it by pointing its predecessor at its successor.
	 */


	if(httpServer_listen(&server, arguments_vector[1], 32)){
		server.listeningSocket_fileDescriptor = -1;
		httpServer_close(&server);
		return 5;
	}

	if(2 < argument_count)
		if(kill(atoi(arguments_vector[2]), SIGCONT)){
			httpServer_close(&server);
			return 13;
		}

	while(1){
		if(fd_canReadp(server.listeningSocket_fileDescriptor))
			httpServer_acceptNewConnection(&server);
		else
			if(!httpServer_stepConnections(&server))
				usleep(100);
	}

	if(httpServer_close(&server)){
		server.listeningSocket_fileDescriptor = -1;
		return 6;
	}
	return 0;
}
