/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <string.h>
#include "./static.h"
#include "./request.h"

int match_resource_url(struct staticGetResource *resource, struct extent *url, struct linked_list *node){
	(void)node;
	node = 0;
	if(!resource) return 0;
	if(!url) return 0;
	if(!(resource->url)) return 0;
	return extent_url_equalsp(resource->url, url);
}


int staticGetResource_canRespondp(struct httpResource *resource, struct incomingHttpRequest *request){
	if(!resource) return 0;
	if(!request) return 0;
	if(!(request->input.method)) return 0;
	if(!(request->input.requestUrl)) return 0;
	if(-1 == request->input.httpMajorVersion) return 0;
	if(-1 == request->input.httpMinorVersion) return 0;
	if(request->done_writing) return 0;
	return 1;
}

int staticGetResource_respond(struct httpResource *resource, struct incomingHttpRequest *connection){
	struct extent reason;
	struct staticGetResource *response;
	if(!resource) return 1;
	if(!connection) return 1;
	response = (struct staticGetResource*)(resource->context);
	if(!response) return 1;
	if(!(connection->input.method))
		return incomingHttpRequest_respond_badMethod(connection);
	if(3 != connection->input.method->len)
		return incomingHttpRequest_respond_badMethod(connection);
	if(!(connection->input.method->bytes))
		return incomingHttpRequest_respond_badMethod(connection);
	if(strncmp(connection->input.method->bytes, "GET", connection->input.method->len + 1))
		return incomingHttpRequest_respond_badMethod(connection);
	point_extent_at_nice_string(&reason, "OK");
	return incomingHttpRequest_sendResponse(connection, 200, &reason, response->headers, response->body);
}


int contiguousHtmlResource_init(struct contiguousHtmlResource *res, char *url, char *body){
	if(!res) return 2;
	if(!url) return 3;
	if(!body) return 3;
	if(point_extent_at_nice_string(&(res->url), url)) return 4;
	if(point_extent_at_nice_string(&(res->body), body)) return 4;
	res->staticResource.url = &(res->url);
	res->staticResource.body = &(res->body);
	res->staticResource.headers = push_header_nice_strings(&(res->headerTop), &(res->keyNode), &(res->valNode), &(res->key), "Content-Type", &(res->val), "text/html", 0);
	point_extent_at_nice_string(&(res->resource.url), url);
	return !(res->staticResource.headers);
}
