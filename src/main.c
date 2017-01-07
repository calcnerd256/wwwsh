/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <unistd.h>
#include "./server.h"
#include "./request.h"
#include "./static.h"
#include "./form.h"


int sampleForm_respond_POST(struct httpResource *res, struct incomingHttpRequest *req){
	if(!res) return 1;
	if(!req) return 1;
	printf("request body (%d byte(s)): {\n", requestInput_getBodyLengthSoFar(&(req->input)));
	requestInput_consumeLastLine(&(req->input));
	requestInput_printBody(&(req->input));
	printf("}\n");
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
	struct linked_list formTailCell;
	struct linked_list fieldHead;
	struct linked_list fieldNameNode;
	struct linked_list fieldTagNode;
	struct extent fieldName;
	struct extent fieldTag;
	struct form formForm;

	if(2 != argument_count) return 1;
	if(staticFormResource_init(&formResource, &formForm, "/formtest/", "form"))
		return 10;
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
	formTailCell.data = &formForm;
	formTailCell.next = &formContext;

	fieldHead.next = 0;
	fieldHead.data = &fieldNameNode;
	fieldNameNode.data = &fieldName;
	fieldNameNode.next = &fieldTagNode;
	fieldTagNode.data = &fieldTag;
	fieldTagNode.next = 0;

	formForm.respond_POST = &sampleForm_respond_POST;
	formForm.fields = &fieldHead;
	formResource.context = &formTailCell;
	if(httpServer_pushResource(&server, &(formResource.node), &(formResource.resource), &staticFormResource_urlMatchesp, &staticFormResource_canRespondp, &staticFormResource_respond, &formResource))
		return 7;

	if(httpServer_listen(&server, arguments_vector[1], 32)){
		server.listeningSocket_fileDescriptor = -1;
		httpServer_close(&server);
		return 5;
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
