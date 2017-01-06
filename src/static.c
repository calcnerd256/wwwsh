/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <string.h>
#include "./static.h"
#include "./request.h"

int match_resource_url(struct staticGetResource *resource, struct extent *url, struct linked_list *node){
	(void)node;
	if(!resource) return 0;
	if(!url) return 0;
	if(!(resource->url)) return 0;
	if(resource->url->len != url->len) return 0;
	return !strncmp(resource->url->bytes, url->bytes, url->len + 1);
}

int staticGetResource_urlMatchesp(struct httpResource *resource, struct extent *url){
	if(!resource) return 0;
	if(!(resource->context)) return 0;
	if(!url) return 0;
	return match_resource_url(resource->context, url, 0);
}

int staticGetResource_respond(struct httpResource *resource, struct incomingHttpRequest *connection){
	struct extent reason;
	struct staticGetResource *response;
	if(!resource) return 1;
	if(!connection) return 1;
	response = (struct staticGetResource*)(resource->context);
	if(!response) return 1;
	if(!(connection->input.method))
		return connection_bundle_respond_bad_method(connection);
	if(3 != connection->input.method->len)
		return connection_bundle_respond_bad_method(connection);
	if(!(connection->input.method->bytes))
		return connection_bundle_respond_bad_method(connection);
	if(strncmp(connection->input.method->bytes, "GET", connection->input.method->len + 1))
		return connection_bundle_respond_bad_method(connection);
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
	return !(res->staticResource.headers);
}
