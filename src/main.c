/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <unistd.h>
#include <string.h>
#include "./server.h"
#include "./request.h"
#include "./static.h"


int sampleForm_urlMatchesp(struct httpResource *res, struct extent *url){
	if(!res) return 0;
	res = 0;
	if(!url) return 0;
	if(!(url->bytes)) return 0;
	if(!(url->len)) return 0;
	if(url->len < 9) return 0;
	if(url->len > 10) return 0;
	if(strncmp("/formtest", url->bytes, 9)) return 0;
	if(url->bytes[9] == '/') return 1;
	res = 0;
	return !(url->bytes[9]);
}
int sampleForm_canRespondp(struct httpResource *res, struct incomingHttpRequest *req){
	if(!res) return 0;
	if(!req) return 0;
	/* TODO: check the request method */
	return 0;
}
int sampleForm_respond(struct httpResource *res, struct incomingHttpRequest *req){
	if(!res) return 1;
	if(!req) return 1;
	/* TODO: capture the whole request body */
	return 1;
}

int main(int argument_count, char* *arguments_vector){
	struct httpServer server;
	int status = 0;

	struct contiguousHtmlResource rootResourceStorage;

	if(2 != argument_count) return 1;
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
		"  <p>Coming soon: a resource for the following form to POST to</p>\r\n"
		"  <form method=\"POST\" action=\"./formtest/\">\r\n"
		"   <input type=\"submit\" />\r\n"
		"  </form>\r\n"
		" </body>\r\n"
		"</html>\r\n"
		"\r\n"
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
