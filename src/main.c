/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "./server.h"
#include "./request.h"
#include "./static.h"
#include "./form.h"

int spawn_child(int input, int output, char* cmd){
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

int sampleForm_respond_POST(struct httpResource *res, struct incomingHttpRequest *req, struct linked_list *formData){
	pid_t kid;
	int ki[2];
	int ko[2];
	int activity;
	char buf[256];
	int chunkLen;
	if(!res) return 1;
	if(!req) return 1;
	if(!formData) return 1;
	if(pipe2(ki, O_CLOEXEC)) return 1;
	if(pipe2(ko, O_CLOEXEC)){
		close(ki[0]);
		close(ki[1]);
		return 1;
	}
	printf("forking\n");
	kid = fork();
	printf("forked %d\n", kid);
	if(kid < 0){
		close(ki[0]);
		close(ki[1]);
		close(ko[0]);
		close(ko[1]);
		return 1;
	}
	if(!kid){
		close(ki[1]);
		close(ko[0]);
		if(spawn_child(ki[0], ko[1], "ls -al")){
			close(ki[0]);
			close(ko[1]);
			_exit(1);
			return 1;
		}
	}
	/* TODO: store the process in a structure */
	close(ki[0]);
	close(ko[1]);

	close(ki[1]);
	buf[0] = 0;
	while(1){
		activity = 0;
		if(waitpid(kid, 0, WNOHANG)) activity = 1;
		buf[255] = 0;
		chunkLen = read(ko[0], buf, 255);
		if(-1 == chunkLen)
			break;
		if(!chunkLen)
			break;
		buf[chunkLen] = 0;
		activity = 1;
		printf("read chunk from kid <%s>\n", buf);
		if(!activity)
			usleep(10);
	}
	close(ko[0]);
	waitpid(kid, 0, 0);
	return staticFormResource_respond_GET(res, req);
}

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
		"   <a href=\"./formtest/\">a form</a>\r\n"
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

	status = staticFormResource_init(&formResource, &server, &formForm, "/formtest/", "form", &fieldHead, &sampleForm_respond_POST, &formContext);
	if(status)
		return 7;

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
		if(httpServer_canAcceptConnectionp(&server))
			httpServer_acceptNewConnection(&server);
		else
			if(!httpServer_stepConnections(&server))
				usleep(10);
	}

	if(httpServer_close(&server)){
		server.listeningSocket_fileDescriptor = -1;
		return 6;
	}
	return 0;
}
