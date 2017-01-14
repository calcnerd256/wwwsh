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


struct childProcessResource{
	struct mempool allocations;
	struct chunkStream outputStream;
	struct linked_list *node;
	pid_t pid;
	int input;
	int output;
};

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
		waitpid(kid->pid, 0, 0);
	kid->pid = -1;
	free_pool(&(kid->allocations));
	kid->node->data = 0;
	return 0;
}

int childProcessResource_steppedp(struct childProcessResource *kid){
	char buf[CHUNK_SIZE + 1];
	int chunkLen;
	if(!kid) return 0;
	if(-1 == kid->pid) return 0;
	if(kid->pid == waitpid(kid->pid, 0, WNOHANG)){
		childProcessResource_close(kid);
		return 1;
	}
	if(-1 == kid->output) return 0;
	if(!fd_canReadp(kid->output)) return 0;
	buf[0] = 0;
	buf[CHUNK_SIZE] = 0;
	chunkLen = read(kid->output, buf, CHUNK_SIZE);
	if(-1 == chunkLen){
		close(kid->output);
		kid->output = -1;
		return 1;
	}
	buf[chunkLen] = 0;
	if(!chunkLen){
		close(kid->output);
		kid->output = -1;
		return 1;
	}
	chunkStream_append(&(kid->outputStream), buf, chunkLen);
	return 1;
}

int spawnForm_respond_POST(struct httpResource *res, struct incomingHttpRequest *req, struct linked_list *formData){
	struct childProcessResource *child;
	struct httpServer *server;
	struct staticFormResource *fr;
	if(!res) return 1;
	if(!req) return 1;
	if(!formData) return 1;
	fr = res->context;
	if(!fr) return 1;
	server = ((struct linked_list*)(fr->context))->data;
	if(!server) return 1;
	child = malloc(sizeof(struct childProcessResource));
	if(childProcessResource_init_spawn(child, "ls -al")) return 1;
	child->node = malloc(sizeof(struct linked_list));
	child->node->next = server->children;
	child->node->data = child;
	server->children = child->node;

	close(child->input);
	child->input = -1;
	while(-1 != child->output)
		if(!childProcessResource_steppedp(child))
			usleep(100);
	if(-1 != child->pid)
		waitpid(child->pid, 0, 0);

	memset(child, 0, sizeof(struct childProcessResource));
	free(child);

	return staticFormResource_respond_GET(res, req);
}

/*
	TODO
	make childProcessResource a resource
	make process spawner respond with the child's URL
	push childProcessResource onto server's resource stack upon spawn
	delete childProcessResource from server's resources on close
	move childProcessResource class into its own compilation unit
	free childProcessResource on close
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
