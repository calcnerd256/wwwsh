/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <string.h>
#include <stdlib.h>
#include "./request.h"
#include "./server.h"
#include "./static.h"


int staticFormResource_urlMatchesp(struct httpResource *res, struct extent *url){
	if(!res) return 0;
	if(!url) return 0;
	if(!(res->context)) return 0;
	return extent_url_equalsp(&(((struct staticFormResource*)(res->context))->url), url);
}

int match_header_key(struct linked_list *header, struct extent *target, struct linked_list *node){
	struct extent *key;
	if(!node) return 0;
	if(!header) return 0;
	if(!target) return 0;
	key = (struct extent*)(header->data);
	if(!key) return 0;
	if(key->len != target->len) return 0;
	return !strncmp(target->bytes, key->bytes, target->len);
}
int staticFormResource_canRespondp(struct httpResource *res, struct incomingHttpRequest *req){
	int contentLength = -1;
	struct linked_list *match_node;
	struct extent contentLengthKey;
	if(!res) return 0;
	if(!req) return 0;
	if(point_extent_at_nice_string(&contentLengthKey, "Content-Length")) return 0;
	if(!staticGetResource_canRespondp(res, req)) return 0;
	if(!(req->input.method)) return 0;
	if(3 > req->input.method->len) return 1;
	if(!strncmp("GET", req->input.method->bytes, 4)) return 1;
	if(4 > req->input.method->len) return 1;
	if(strncmp("POST", req->input.method->bytes, 5)) return 1;
	if(!(req->input.headersDone)) return 0;
	if(!(req->input.headers)) return 0;

	if(first_matching(req->input.headers->head, (visitor_t)(&match_header_key), (void*)(&contentLengthKey), &match_node)) return 0;
	contentLength = atoi(((struct extent*)(((struct linked_list*)(match_node->data))->next->data))->bytes);

	if(-1 == contentLength) return 0;
	if(contentLength > requestInput_getBodyLengthSoFar(&(req->input))) return 0;
	return 1;
}

int staticFormResource_writelnField(struct incomingHttpRequest *req, struct extent *tag, struct extent *name, struct linked_list *rest){
	int status = 0;
	char singleton = 0;
	char isInput = 0;
	struct extent *inputType = 0;
	if(!req) return 1;
	if(!name) return 2;
	if(!tag) return 2;
	if(!(tag->bytes)) return 2;
	if(5 == tag->len)
		if(!strncmp(tag->bytes, "input", 5))
			isInput = 1;
	if(isInput) singleton = 1;
	if(isInput)
		if(rest)
			inputType = (struct extent*)(rest->data);
	status += !!incomingHttpRequest_writeChunk_niceString(req, "<");
	status += !!incomingHttpRequest_writeChunk_htmlSafeExtent(req, tag);
	status += !!incomingHttpRequest_writeChunk_niceString(req, " name=\"");
	status += !!incomingHttpRequest_writeChunk_htmlSafeExtent(req, name);
	status += !!incomingHttpRequest_writeChunk_niceString(req, "\"");
	if(inputType){
		status += !!incomingHttpRequest_writeChunk_niceString(req, " type=\"");
		status += !!incomingHttpRequest_writeChunk_htmlSafeExtent(req, inputType);
		status += !!incomingHttpRequest_writeChunk_niceString(req, "\"");
	}
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
	status += !!staticFormResource_writelnField(context, tag, name, field->next->next);
	return status;
}
int form_respond_GET(struct form *form, struct incomingHttpRequest *req, struct extent *action){
	int status = 0;
	if(!form) return 1;
	if(!req) return 1;
	if(!action) return 1;
	if(!(form->title)) return 2;
	if(!(action->bytes))
		if(action->len) return 2;

	if(incomingHttpRequest_beginChunkedHtmlBody(req, 0, form->title->bytes, form->title->len))
		return 3;
	status += !!incomingHttpRequest_writeChunk_niceString(req, "  <h1>");
	status += !!incomingHttpRequest_writeChunk_htmlSafeExtent(req, form->title);
	status += !!incomingHttpRequest_writelnChunk_niceString(req, "</h1>");

	status += !!incomingHttpRequest_writeChunk_niceString(req, "  <form method=\"POST\" action=\"");
	status += !!incomingHttpRequest_write_chunk(req, action->bytes, action->len);
	status += !!incomingHttpRequest_writelnChunk_niceString(req, "\">");

	status += !!traverse_linked_list(form->fields, (visitor_t)(&visit_field_writeOut), req);

	status += !!incomingHttpRequest_writeChunk_niceString(req, "   <input type=\"submit\" value=\"");
	status += !!incomingHttpRequest_writeChunk_htmlSafeExtent(req, form->title);
	status += !!incomingHttpRequest_writelnChunk_niceString(req, "\" />");

	status += !!incomingHttpRequest_writelnChunk_niceString(req, "  </form>");
	if(incomingHttpRequest_endChunkedHtmlBody(req, 0)) return 4;
	if(status) return 5;
	return 0;
}
int staticFormResource_respond_GET(struct httpResource *res, struct incomingHttpRequest *req, struct form *form){
	struct staticFormResource *fr;
	if(!res) return 1;
	if(!req) return 1;
	if(!form) return 1;
	fr = (struct staticFormResource*)(res->context);
	if(!fr) return 2;
	if(form_respond_GET(form, req, &(fr->url))) return 3;
	return 0;
}

int extent_urlDecode(struct extent *str){
	char *r;
	char *w;
	size_t cursor = 0;
	size_t new_len = 0;
	char hex;
	char value;
	if(!str) return 1;
	r = str->bytes;
	w = str->bytes;
	while(cursor < str->len){
		*w = *r;
		if('+' == *r)
			*w = ' ';
		if('%' == *r){
			r++;
			cursor++;
			hex = *r;
			value = 0;
			if('1' <= hex)
				if('9' >= hex)
					value = hex - '1' + 1;
			if('a' <= hex)
				if('f' >= hex)
					value = hex - 'a' + 10;
			if('A' <= hex)
				if('F' >= hex)
					value = hex - 'A' + 10;
			value <<= 4;

			r++;
			cursor++;
			hex = *r;
			if('1' <= hex)
				if('9' >= hex)
					value += hex - '1' + 1;
			if('a' <= hex)
				if('f' >= hex)
					value += hex - 'a' + 10;
			if('A' <= hex)
				if('F' >= hex)
					value += hex - 'A' + 10;

			*w = value;
		}
		r++;
		w++;
		cursor++;
		new_len++;
	}
	*w = 0;
	str->len = new_len;
	return 0;
}
int dequoid_append_fieldData(struct dequoid *fields, struct extent *unparsed, struct mempool *pool){
	char* ptr;
	struct linked_list *field;
	struct extent *kv;
	int toDelimiter;
	int status = 0;
	ptr = palloc(pool, unparsed->len + 1 + 2 * sizeof(struct extent) + 3 * sizeof(struct linked_list));

	field = (struct linked_list*)(ptr + sizeof(struct linked_list));
	status += !!dequoid_append(fields, field, (struct linked_list*)ptr);
	ptr += 2 * sizeof(struct linked_list);

	field->next = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	field->data = ptr;
	ptr += sizeof(struct extent);
	field->next->data = ptr;
	ptr += sizeof(struct extent);

	memcpy(ptr, unparsed->bytes, unparsed->len);
	kv = (struct extent*)(field->data);
	kv->bytes = ptr;
	kv->len = unparsed->len;
	ptr += unparsed->len;
	*ptr = 0;
	ptr = 0;

	for(toDelimiter = 0; (size_t)toDelimiter < unparsed->len; toDelimiter++)
		if('=' == kv->bytes[toDelimiter]){
			kv->bytes[toDelimiter] = 0;
			kv->len = toDelimiter;
			extent_urlDecode(kv);
			break;
		}

	if(kv->len == unparsed->len)
		field->next->data = 0;
	else{
		toDelimiter = kv->len;
		kv = (struct extent*)(field->next->data);
		kv->bytes = ((struct extent*)(field->data))->bytes + toDelimiter + 1;
		kv->len = unparsed->len - toDelimiter - 1;
		extent_urlDecode(kv);
	}

	return 0;
}
int chunkStream_reset(struct chunkStream *stream){
	if(!stream) return 1;
	stream->cursor_chunk = stream->chunk_list.head;
	stream->cursor_chunk_offset = 0;
	return 0;
}
int staticFormResource_respond_POST(struct httpResource *res, struct incomingHttpRequest *req, struct form *form){
	struct dequoid form_data;
	struct extent current;
	int toNextDelimiter;
	struct linked_list *match_node;
	struct extent *contentType;
	if(!res) return 1;
	if(!req) return 1;
	if(!form) return 1;
	if(!(form->respond_POST))
		return incomingHttpRequest_respond_badMethod(req);

	if(point_extent_at_nice_string(&current, "Content-Type")) return 1;
	if(first_matching(req->input.headers->head, (visitor_t)(&match_header_key), (void*)(&current), &match_node))
		return 1; /* TODO: respond to tell the client that the header is missing */
	contentType = (struct extent*)(((struct linked_list*)(match_node->data))->next->data);
	if(contentType->len != 33)
		return 1; /* TODO: respond to tell the client that the header is missing */

	/*
		https://www.w3.org/TR/html401/interact/forms.html#h-17.13.4.1
	*/
	if(strncmp(contentType->bytes, "application/x-www-form-urlencoded", contentType->len))
		/* TODO: support multipart/form-data */
		return 1; /* TODO: respond to tell the client that the header is missing */

	requestInput_consumeLastLine(&(req->input));
	chunkStream_reset(req->input.body);
	dequoid_init(&form_data);
	toNextDelimiter = chunkStream_findByteOffsetFrom(req->input.body, '&', 0);
	while(-1 != toNextDelimiter){
		chunkStream_takeBytes(req->input.body, toNextDelimiter, &current);
		chunkStream_seekForward(req->input.body, 1);
		dequoid_append_fieldData(&form_data, &current, &(req->allocations));
		toNextDelimiter = chunkStream_findByteOffsetFrom(req->input.body, '&', 0);
	}
	toNextDelimiter = chunkStream_lengthRemaining(req->input.body);
	chunkStream_takeBytes(req->input.body, toNextDelimiter, &current);
	dequoid_append_fieldData(&form_data, &current, &(req->allocations));
	return (*(form->respond_POST))(res, req, form_data.head);
}

int staticFormResource_respond(struct httpResource *res, struct incomingHttpRequest *req){
	struct staticFormResource *fr;
	struct form *form;
	if(!res) return 1;
	if(!req) return 1;
	if(!staticFormResource_canRespondp(res, req)) return 1;
	if(!(req->input.method)) return 1;
	if(3 > req->input.method->len)
		return incomingHttpRequest_respond_badMethod(req);
	if(!(res->context)) return 1;
	fr = (struct staticFormResource*)(res->context);
	if(!(fr->context)) return 1;
	form = (struct form*)(fr->form);
	if(!strncmp("GET", req->input.method->bytes, 4))
		return staticFormResource_respond_GET(res, req, form);
	if(4 > req->input.method->len)
		return incomingHttpRequest_respond_badMethod(req);
	if(!strncmp("POST", req->input.method->bytes, 5))
		return staticFormResource_respond_POST(res, req, form);
	return incomingHttpRequest_respond_badMethod(req);
}

int staticFormResource_init(struct staticFormResource *resource, struct httpServer *server, struct form *form, char* url, char* title, struct linked_list *fields, int (*respond_POST)(struct httpResource*, struct incomingHttpRequest*, struct linked_list*), void *context){
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
	if(point_extent_at_nice_string(&(form->action->url), url)) return 2;
	return httpServer_pushResource(server, &(resource->node), form->action, &staticFormResource_urlMatchesp, &staticFormResource_canRespondp, &staticFormResource_respond, resource);
}
