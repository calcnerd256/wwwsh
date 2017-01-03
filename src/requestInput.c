/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "./requestInput.h"

int requestInput_init(struct requestInput *req, struct mempool *pool){
	char *p = 0;
	if(!req) return 1;
	req->done = 0;
	req->httpMajorVersion = -1;
	req->httpMinorVersion = -1;
	req->pool = pool;
	p = palloc(req->pool, 2 * sizeof(struct chunkStream) + sizeof(struct dequoid));
	req->chunks = (struct chunkStream*)p;
	p += sizeof(struct chunkStream);
	req->body = (struct chunkStream*)p;
	p += sizeof(struct chunkStream);
	req->headers = (struct dequoid*)p;
	chunkStream_init(req->chunks, req->pool);
	chunkStream_init(req->body, req->pool);
	req->headersDone = 0;
	dequoid_init(req->headers);
	req->method = 0;
	req->requestUrl = 0;
	req->length = 0;
	req->fd = -1;
	req->done = 0;
	p = 0;
	pool = 0;
	req = 0;
	return 0;
}

int requestInput_findCrlfOffset(struct requestInput *req){
	int offset = 0;
	if(!req) return -1;
	while(1){
		offset = chunkStream_findByteOffsetFrom(req->chunks, '\r', offset);
		if(-1 == offset) return -1;
		if('\n' == chunkStream_byteAtRelativeOffset(req->chunks, offset + 1))
			return offset + 2;
		offset++;
	}
}

int requestInput_printHeaders(struct requestInput *req){
	struct linked_list *node;
	struct linked_list *head;
	struct extent *key;
	struct extent *value;
	if(!req) return 1;
	if(!(req->headers)) return 1;
	node = req->headers->head;
	while(node){
		head = (struct linked_list*)(node->data);
		node = node->next;
		key = (struct extent*)(head->data);
		value = (struct extent*)(head->next->data);
		printf("key=<%s> value=<%s>\n", key->bytes, value->bytes);
	}
	return 0;
}

int requestInput_consumeHeader(struct requestInput *req){
	int colon = 0;
	int toCrlf = 0;
	struct extent key;
	struct extent value;
	struct linked_list *node;
	char *ptr = 0;
	if(!req) return 1;
	if(req->headersDone) return 0;
	if(chunkStream_reduceCursor(req->chunks)) return 1;
	if('\r' == chunkStream_byteAtRelativeOffset(req->chunks, 0)){
		if('\n' != chunkStream_byteAtRelativeOffset(req->chunks, 1))
			return 1;
		req->headersDone = 1;
		chunkStream_seekForward(req->chunks, 2);
		return 0;
	}
	colon = chunkStream_findByteOffsetFrom(req->chunks, ':', 0);
	if(-1 == colon) return 1;
	if(' ' != chunkStream_byteAtRelativeOffset(req->chunks, colon + 1))
		return 1;
	toCrlf = chunkStream_findByteOffsetFrom(req->chunks, '\r', colon + 2);
	if('\n' != chunkStream_byteAtRelativeOffset(req->chunks, colon + toCrlf + 3))
		return 1;
	if(chunkStream_takeBytes(req->chunks, colon, &key)) return 2;
	chunkStream_seekForward(req->chunks, 2);
	if(chunkStream_takeBytes(req->chunks, toCrlf, &value)) return 2;
	chunkStream_seekForward(req->chunks, 2);
	ptr = palloc(req->pool, 3 * sizeof(struct linked_list) + 2 * sizeof(struct extent));
	node = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	dequoid_append(req->headers, (struct linked_list*)ptr, node);
	node = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	node->next = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	node->data = ptr;
	ptr += sizeof(struct extent);
	node->next->data = ptr;
	ptr = 0;
	node->next->next = 0;
	memcpy(node->data, &key, sizeof(struct extent));
	memcpy(node->next->data, &value, sizeof(struct extent));
	node = 0;
	return 0;
}

int requestInput_consumeLine(struct requestInput *req){
	struct extent cursor;
	int offset = 0;
	if(!req) return 1;
	offset = requestInput_findCrlfOffset(req);
	if(-1 == offset) return 1;
	if(chunkStream_takeBytes(req->chunks, offset, &cursor)) return 2;
	return chunkStream_append(req->body, cursor.bytes, cursor.len);
}

int requestInput_consumeLastLine(struct requestInput *req){
	int len;
	struct linked_list *current;
	struct extent cursor;
	len = ((struct extent*)(req->chunks->cursor_chunk->data))->len;
	len -= req->chunks->cursor_chunk_offset;
	current = req->chunks->cursor_chunk->next;
	while(current){
		len += ((struct extent*)(current->data))->len;
		current = current->next;
	}
	if(!len) return 0;
	chunkStream_takeBytes(req->chunks, len, &cursor);
	return chunkStream_append(req->body, cursor.bytes, cursor.len);
}

int requestInput_consumeMethod(struct requestInput *req){
	int len;
	struct extent storage;
	if(!req) return 1;
	if(req->method) return 0;
	len = chunkStream_findByteOffsetFrom(req->chunks, ' ', 0);
	if(-1 == len) return 1;
	if(chunkStream_takeBytes(req->chunks, len, &storage)) return 2;
	req->method = (struct extent*)palloc(req->pool, sizeof(struct extent));
	req->method->bytes = storage.bytes;
	req->method->len = storage.len;
	chunkStream_seekForward(req->chunks, 1);
	return 0;
}

int requestInput_consumeRequestUrl(struct requestInput *req){
	int len;
	struct extent storage;
	if(!req) return 1;
	if(req->requestUrl) return 0;
	len = chunkStream_findByteOffsetFrom(req->chunks, ' ', 0);
	if(-1 == len) return 1;
	if(chunkStream_takeBytes(req->chunks, len, &storage)) return 2;
	req->requestUrl = (struct extent*)palloc(req->pool, sizeof(struct extent));
	req->requestUrl->bytes = storage.bytes;
	req->requestUrl->len = storage.len;
	chunkStream_seekForward(req->chunks, 1);
	return 0;
}

int requestInput_consumeHttpVersion(struct requestInput *req){
	int len;
	struct extent storage;
	int ver;
	if(!req) return 1;
	if(-1 != req->httpMajorVersion) return 0;
	len = chunkStream_findByteOffsetFrom(req->chunks, '/', 0);
	if(4 != len) return 1;
	if(chunkStream_takeBytes(req->chunks, 5, &storage)) return 2;
	if(strncmp(storage.bytes, "HTTP/", storage.len + 1)) return 3;
	len = chunkStream_findByteOffsetFrom(req->chunks, '.', 0);
	if(-1 == len) return 4;
	if(len >= requestInput_findCrlfOffset(req)) return 5;
	if(chunkStream_takeBytes(req->chunks, len, &storage)) return 6;
	ver = atoi(storage.bytes);

	chunkStream_seekForward(req->chunks, 1);
	len = requestInput_findCrlfOffset(req);
	if(len < 2) return 7;
	if(chunkStream_takeBytes(req->chunks, len - 2, &storage)) return 8;
	req->httpMinorVersion = atoi(storage.bytes);
	req->httpMajorVersion = ver;
	chunkStream_seekForward(req->chunks, 2);
	return 0;
}

int requestInput_processStep(struct requestInput *req){
	/*
		https://tools.ietf.org/html/rfc7230#section-3.1.1
	*/
	int status = 0;
	if(!req) return 0;
	if(!(req->method)){
		if(requestInput_consumeMethod(req)) return 2;
		status = 1;
	}
	if(!(req->requestUrl)){
		if(requestInput_consumeRequestUrl(req)) return status ? status : 2;
		status = 1;
	}
	if(-1 == req->httpMinorVersion){
		if(requestInput_consumeHttpVersion(req)) return status ? status : 2;
		status = 1;
	}
	while(!(req->headersDone)){
		if(!requestInput_consumeHeader(req))
			status = 1;
		else
			return status ? status : 2;
	}
	while(!requestInput_consumeLine(req))
		status = 1;
	if(req->done){
		requestInput_consumeLastLine(req);
		status = 1;
	}
	chunkStream_reduceCursor(req->chunks);
	return status;
}

int requestInput_printBody(struct requestInput *req){
	struct linked_list *node;
	struct extent *current;
	if(!req) return 1;
	node = req->body->chunk_list.head;
	while(node){
		current = (struct extent*)(node->data);
		printf("%p\t%s", node->data, current->bytes);
		if(!(node->next))
			if('\n' != current->bytes[current->len - 1])
				printf("\n");
		node = node->next;
	}
	return 0;
}
