/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <string.h>
#include <stdlib.h>
#include "./form.h"
#include "./request.h"
#include "./server.h"
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

int staticFormResource_writelnField(struct incomingHttpRequest *req, struct extent *tag, struct extent *name){
	int status = 0;
	char singleton = 0;
	if(!req) return 1;
	if(!name) return 2;
	if(!tag) return 2;
	if(!(tag->bytes)) return 2;
	if(5 == tag->len)
		if(!strncmp(tag->bytes, "input", 5))
			singleton = 1;
	status += !!incomingHttpRequest_writeChunk_niceString(req, "<");
	status += !!incomingHttpRequest_writeChunk_htmlSafeExtent(req, tag);
	status += !!incomingHttpRequest_writeChunk_niceString(req, " name=\"");
	status += !!incomingHttpRequest_writeChunk_htmlSafeExtent(req, name);
	status += !!incomingHttpRequest_writeChunk_niceString(req, "\"");
	if(singleton)
		status += !!incomingHttpRequest_writeChunk_niceString(req, " ");
	else
		status += !!incomingHttpRequest_writeChunk_niceString(req, "><");
	status += !!incomingHttpRequest_writeChunk_niceString(req, "/");
	if(!singleton)
		status += !!incomingHttpRequest_writeChunk_htmlSafeExtent(req, tag);
	status += !!incomingHttpRequest_writelnChunk_niceString(req, ">");
	return status;
}
int visit_field_writeOut(struct linked_list *field, struct incomingHttpRequest *context, struct linked_list *node){
	struct extent *name;
	struct extent *tag;
	int status = 0;
	(void)node;
	if(!field) return 0;
	if(!context) return 1;
	if(!(field->next)) return 1;
	name = (struct extent*)(field->data);
	tag = (struct extent*)(field->next->data);
	status += !!incomingHttpRequest_writeChunk_niceString(context, "   ");
	status += !!staticFormResource_writelnField(context, tag, name);
	return status;
}
int staticFormResource_respond_GET(struct httpResource *res, struct incomingHttpRequest *req){
	struct staticFormResource *fr;
	struct form *form;
	int status = 0;
	if(!res) return 1;
	if(!req) return 1;
	fr = (struct staticFormResource*)(res->context);
	if(!fr) return 2;
	form = (struct form*)(fr->form);
	if(!form) return 2;
	if(!(form->title)) return 3;
	if(!(form->fields)) return 3;
	if(!(form->action)) return 3;

	if(incomingHttpRequest_beginChunkedHtmlOk(req, 0)) return 4;
	status = 0;

	status += !!incomingHttpRequest_writelnChunk_niceString(req, "<html>");
	status += !!incomingHttpRequest_writelnChunk_niceString(req, " <head>");
	status += !!incomingHttpRequest_writelnChunk_niceString(req, "  <title>");

	status += !!incomingHttpRequest_writeChunk_niceString(req, "   ");
	if(status) return 5;
	if(incomingHttpRequest_writeChunk_htmlSafeExtent(req, form->title)) return 6;
	status += !!incomingHttpRequest_writelnChunk_niceString(req, "");

	status += !!incomingHttpRequest_writelnChunk_niceString(req, "  </title>");
	status += !!incomingHttpRequest_writelnChunk_niceString(req, " </head>");
	status += !!incomingHttpRequest_writelnChunk_niceString(req, " <body>");

	status += !!incomingHttpRequest_writeChunk_niceString(req, "  <h1>");
	if(status) return 7;
	if(incomingHttpRequest_writeChunk_htmlSafeExtent(req, form->title)) return 8;
	status += !!incomingHttpRequest_writelnChunk_niceString(req, "</h1>");

	status += !!incomingHttpRequest_writeChunk_niceString(req, "  <form method=\"POST\" action=\"");
	if(incomingHttpRequest_write_chunk(req, fr->url.bytes, fr->url.len))
		return 9;
	status += !!incomingHttpRequest_writelnChunk_niceString(req, "\">");

	if(traverse_linked_list(form->fields, (visitor_t)(&visit_field_writeOut), req))
		return 10;

	status += !!incomingHttpRequest_writeChunk_niceString(req, "   <input type=\"submit\" value=\"");
	if(status) return 11;
	if(incomingHttpRequest_writeChunk_htmlSafeExtent(req, form->title)) return 12;
	status += !!incomingHttpRequest_writelnChunk_niceString(req, "\" />");

	status += !!incomingHttpRequest_writelnChunk_niceString(req, "  </form>");
	status += !!incomingHttpRequest_writelnChunk_niceString(req, " </body>");
	status += !!incomingHttpRequest_writelnChunk_niceString(req, "</html>");
	if(status) return 13;

	if(incomingHttpRequest_sendLastChunk(req, 0)) return 14;
	return 0;
}

int staticFormResource_respond(struct httpResource *res, struct incomingHttpRequest *req){
	struct staticFormResource *fr;
	struct form *form;
	if(!res) return 1;
	if(!req) return 1;
	if(!staticFormResource_canRespondp(res, req)) return 1;
	if(!(req->input.method)) return 1;
	if(!(res->context)) return 1;
	fr = (struct staticFormResource*)(res->context);
	if(!(fr->context)) return 1;
	form = (struct form*)(fr->form);
	if(3 > req->input.method->len)
		return incomingHttpRequest_respond_badMethod(req);
	if(!strncmp("GET", req->input.method->bytes, 4))
		return staticFormResource_respond_GET(res, req);
	if(4 > req->input.method->len)
		return incomingHttpRequest_respond_badMethod(req);
	if(!strncmp("POST", req->input.method->bytes, 5)){
		if(!(form->respond_POST))
			return incomingHttpRequest_respond_badMethod(req);
		return (*(form->respond_POST))(res, req);
	}
	return incomingHttpRequest_respond_badMethod(req);
}

int staticFormResource_init(struct staticFormResource *resource, struct httpServer *server, struct form *form, char* url, char* title, struct linked_list *fields, int (*respond_POST)(struct httpResource*, struct incomingHttpRequest*), void *context){
	if(!resource) return 1;
	if(!form) return 1;
	if(!url) return 1;
	if(!title) return 1;
	form->action = &(resource->resource);
	form->title = &(resource->title);
	form->fields = fields;
	form->respond_POST = respond_POST;
	resource->form = form;
	resource->context = context;
	if(point_extent_at_nice_string(&(resource->title), title)) return 2;
	if(point_extent_at_nice_string(&(resource->url), url)) return 2;
	return httpServer_pushResource(server, &(resource->node), form->action, &staticFormResource_urlMatchesp, &staticFormResource_canRespondp, &staticFormResource_respond, resource);
}
