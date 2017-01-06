/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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
int visit_header_getContentLength(struct linked_list *header, int *contentLength, struct linked_list *node){
	struct extent *key;
	struct extent *value;
	if(!node) return 1;
	if(!header) return 0;
	if(!contentLength) return 1;
	if(-1 != *contentLength) return 0;
	key = (struct extent*)(header->data);
	if(!(header->next)) return 1;
	value = (struct extent*)(header->next->data);
	if(!key) return 1;
	if(!value) return 1;
	if(14 > key->len) return 0;
	if(strncmp("Content-Length", key->bytes, 15)) return 0;
	*contentLength = atoi(value->bytes);
	return 0;
}
int sampleForm_canRespondp(struct httpResource *res, struct incomingHttpRequest *req){
	int contentLength = -1;
	if(!res) return 0;
	if(!req) return 0;
	if(!staticGetResource_canRespondp(res, req)) return 0;
	if(!(req->input.method)) return 0;
	if(3 > req->input.method->len) return 1;
	if(!strncmp("GET", req->input.method->bytes, 4)) return 1;
	if(4 > req->input.method->len) return 1;
	if(strncmp("POST", req->input.method->bytes, 5)) return 1;
	if(!(req->input.headersDone)) return 0;
	if(!(req->input.headers)) return 0;
	if(traverse_linked_list(req->input.headers->head, (visitor_t)(&visit_header_getContentLength), &contentLength)) return 0;
	if(-1 == contentLength) return 0;
	if(contentLength > requestInput_getBodyLengthSoFar(&(req->input))) return 0;
	return 1;
}
int sampleForm_respond_GET(struct httpResource *res, struct incomingHttpRequest *req){
	struct extent body;
	if(!res) return 1;
	if(!req) return 1;
	if(point_extent_at_nice_string(&body, "<html><body>test</body></html>"))
		return 1;
	return incomingHttpRequest_respond_htmlOk(req, 0, &body);
}
int sampleForm_respond_POST(struct httpResource *res, struct incomingHttpRequest *req){
	if(!res) return 1;
	if(!req) return 1;
	printf("request body (%d byte(s)): {\n", requestInput_getBodyLengthSoFar(&(req->input)));
	requestInput_consumeLastLine(&(req->input));
	requestInput_printBody(&(req->input));
	printf("}\n");
	return sampleForm_respond_GET(res, req);
}
int sampleForm_respond(struct httpResource *res, struct incomingHttpRequest *req){
	if(!res) return 1;
	if(!req) return 1;
	if(!sampleForm_canRespondp(res, req)) return 1;
	if(!(req->input.method)) return 1;
	if(3 > req->input.method->len)
		return incomingHttpRequest_respond_badMethod(req);
	if(!strncmp("GET", req->input.method->bytes, 4))
		return sampleForm_respond_GET(res, req);
	if(4 > req->input.method->len)
		return incomingHttpRequest_respond_badMethod(req);
	if(!strncmp("POST", req->input.method->bytes, 5))
		return sampleForm_respond_POST(res, req);
	return incomingHttpRequest_respond_badMethod(req);
}

int main(int argument_count, char* *arguments_vector){
	struct httpServer server;
	int status = 0;

	struct contiguousHtmlResource rootResourceStorage;

	struct linked_list formHead;
	struct httpResource formResource;

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

	if(httpServer_pushResource(&server, &formHead, &formResource, &sampleForm_urlMatchesp, &sampleForm_canRespondp, &sampleForm_respond, 0)) return 4;

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
