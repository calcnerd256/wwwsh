/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "./form.h"
#include "./request.h"
#include "./static.h"

int staticFormResource_urlMatchesp(struct httpResource *res, struct extent *url){
	struct staticFormResource *fr;
	size_t reqUrlLen = 0;
	size_t resUrlLen = 0;
	if(!res) return 0;
	if(!url) return 0;
	fr = (struct staticFormResource*)(res->context);
	if(!fr) return 0;
	resUrlLen = fr->url.len;
	if(!resUrlLen){
		res = 0;
		fr = 0;
		if(!url->len) return 1;
		if(!url->bytes) return 0;
		if(!(url->bytes[0])) return 1;
		if('/' != url->bytes[0]) return 0;
		if(url->bytes[1]) return 1;
		return 0;
	}
	if(!fr->url.bytes) return 0;
	if('/' == fr->url.bytes[resUrlLen - 1]) resUrlLen--;
	reqUrlLen = url->len;
	if(!reqUrlLen) return !resUrlLen;
	if(!(url->bytes)) return 0;
	if('/' == url->bytes[reqUrlLen - 1]) reqUrlLen--;
	if(resUrlLen != reqUrlLen) return 0;
	return !strncmp(fr->url.bytes, url->bytes, resUrlLen);
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
int staticFormResource_canRespondp(struct httpResource *res, struct incomingHttpRequest *req){
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

int staticFormResource_init(struct staticFormResource *resource, struct form *form, char* url, char* title){
	if(!resource) return 1;
	if(!form) return 1;
	if(!url) return 1;
	if(!title) return 1;
	resource->node.next = 0;
	resource->node.data = &(resource->resource);
	resource->resource.urlMatchesp = &staticFormResource_urlMatchesp;
	resource->resource.canRespondp = &staticFormResource_canRespondp;
	resource->resource.respond = 0;
	resource->resource.context = resource;
	form->title = &(resource->title);
	form->fields = 0;
	form->action = &(resource->resource);
	form->respond_POST = 0;
	resource->form = form;
	resource->context = 0;
	if(point_extent_at_nice_string(&(resource->title), title)) return 1;
	return point_extent_at_nice_string(&(resource->url), url);
}

int sampleForm_respond_GET(struct httpResource *res, struct incomingHttpRequest *req){
	struct extent body;
	struct linked_list header_transferEncoding_node;
	struct linked_list header_transferEncoding_key;
	struct linked_list header_transferEncoding_val;
	struct extent header_transferEncoding_key_str;
	struct extent header_transferEncoding_val_str;
	struct linked_list header_contentType_node;
	struct linked_list header_contentType_key;
	struct linked_list header_contentType_val;
	struct extent header_contentType_key_str;
	struct extent header_contentType_val_str;
	struct extent reason;
	int status = 0;
	if(!res) return 1;
	if(!req) return 1;
	header_transferEncoding_node.data = &header_transferEncoding_key;
	header_transferEncoding_node.next = &header_contentType_node;
	header_contentType_node.data = &header_contentType_key;
	header_contentType_node.next = 0;
	header_transferEncoding_key.data = &header_transferEncoding_key_str;
	header_transferEncoding_key.next = &header_transferEncoding_val;
	header_transferEncoding_val.data = &header_transferEncoding_val_str;
	header_transferEncoding_val.next = 0;
	header_contentType_key.data = &header_contentType_key_str;
	header_contentType_key.next = &header_contentType_val;
	header_contentType_val.data = &header_contentType_val_str;
	header_contentType_val.next = 0;
	if(point_extent_at_nice_string(&header_transferEncoding_key_str, "Transfer-Encoding")) return 2;
	if(point_extent_at_nice_string(&header_transferEncoding_val_str, "chunked")) return 2;
	if(point_extent_at_nice_string(&header_contentType_key_str, "Content-Type")) return 2;
	if(point_extent_at_nice_string(&header_contentType_val_str, "text/html")) return 2;
	if(point_extent_at_nice_string(&reason, "OK")) return 2;
	status = point_extent_at_nice_string(
		&body,
		"<html>\r\n"
		" <head>\r\n"
		"  <title>form</title>\r\n"
		" </head>\r\n"
		" <body>\r\n"
		"  test\r\n"
		"  <form method=\"POST\" action=\"/formtest/\">\r\n"
		"   <textarea name=\"cmd\"></textarea>\r\n"
		"   <input type=\"submit\" value=\"test\" />\r\n"
		"  </form>\r\n"
		" </body>\r\n"
		"</html>\r\n"
	);
	if(status) return 3;
	if(incomingHttpRequest_sendResponseHeaders(req, 200, &reason, &header_transferEncoding_node)) return 4;
	if(incomingHttpRequest_write_chunk(req, body.bytes, body.len)) return 5;
	if(incomingHttpRequest_write_chunk(req, 0, 0)) return 6;
	if(incomingHttpRequest_write_crlf(req)) return 7;
	return incomingHttpRequest_closeWrite(req);
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
	struct linked_list *context;
	struct staticFormResource *fr;
	struct form *form;
	if(!res) return 1;
	if(!req) return 1;
	if(!staticFormResource_canRespondp(res, req)) return 1;
	if(!(req->input.method)) return 1;
	if(!(res->context)) return 1;
	fr = (struct staticFormResource*)(res->context);
	if(!(fr->context)) return 1;
	context = (struct linked_list*)(fr->context);
	if(!(context->data)) return 1;
	form = (struct form*)(context->data);
	if(3 > req->input.method->len)
		return incomingHttpRequest_respond_badMethod(req);
	if(!strncmp("GET", req->input.method->bytes, 4))
		return sampleForm_respond_GET(res, req);
	if(4 > req->input.method->len)
		return incomingHttpRequest_respond_badMethod(req);
	if(!strncmp("POST", req->input.method->bytes, 5)){
		if(!(form->respond_POST))
			return incomingHttpRequest_respond_badMethod(req);
		return (*(form->respond_POST))(res, req);
	}
	return incomingHttpRequest_respond_badMethod(req);
}
