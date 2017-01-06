/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <unistd.h>
#include "./server.h"
#include "./request.h"
#include "./static.h"
#include "./form.h"

int main(int argument_count, char* *arguments_vector){
	struct httpServer server;
	int status = 0;

	struct contiguousHtmlResource rootResourceStorage;

	struct linked_list formHead;
	struct httpResource formResource;
	struct mempool formAllocations;
	struct linked_list formContext;
	struct linked_list formPoolCell;
	struct linked_list formTailCell;
	struct form formForm;
	struct extent formTitle;

	if(2 != argument_count) return 1;
	if(init_pool(&formAllocations)) return 8;
	if(point_extent_at_nice_string(&formTitle, "form")) return 9;
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
		"   a resource responding to the following form's POST requests\r\n"
		"  </p>\r\n"
		"  <form method=\"POST\" action=\"./formtest/\">\r\n"
		"   <textarea name=\"cmd\"></textarea>\r\n"
		"   <input type=\"submit\" value=\"test\" />\r\n"
		"  </form>\r\n"
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
	formPoolCell.next = &formTailCell;
	formTailCell.data = &formForm;
	formTailCell.next = 0;
	formForm.title = &formTitle;
	formForm.fields = 0;
	formForm.action = &formResource;
	if(httpServer_pushResource(&server, &formHead, &formResource, &sampleForm_urlMatchesp, &sampleForm_canRespondp, &sampleForm_respond, &formContext)) return 7;

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
