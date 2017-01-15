/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "./requestInput.h"


int chunkStream_init(struct chunkStream *ptr, struct mempool *pool){
	ptr->pool = pool;
	pool = 0;
	ptr->cursor_chunk = 0;
	ptr->cursor_chunk_offset = 0;
	dequoid_init(&(ptr->chunk_list));
	ptr = 0;
	return 0;
}

int chunkStream_append(struct chunkStream *stream, char *bytes, size_t len){
	char *bufptr;
	struct linked_list *new_head;
	struct extent *chunk;
	if(!len){
		stream = 0;
		bytes = 0;
		len = 0;
		bufptr = 0;
		new_head = 0;
		chunk = 0;
		return 0;
	}
	bufptr = palloc(stream->pool, len + 1 + sizeof(struct extent) + sizeof(struct linked_list));
	new_head = (struct linked_list*)bufptr;
	bufptr += sizeof(struct linked_list);
	chunk = (struct extent*)bufptr;
	bufptr += sizeof(struct extent);
	memcpy(bufptr, bytes, len);
	bytes = 0;
	chunk->bytes = bufptr;
	bufptr += len;
	*bufptr = 0;
	bufptr = 0;
	chunk->len = len;
	len = 0;
	dequoid_append(&(stream->chunk_list), chunk, new_head);
	chunk = 0;
	stream = 0;
	new_head = 0;
	return 0;
}

int chunkStream_overstepCursorp(struct chunkStream *stream){
	if(!stream->cursor_chunk) return 1;
	return ((struct extent*)(stream->cursor_chunk->data))->len <= stream->cursor_chunk_offset;
}
int chunkStream_reduceCursor(struct chunkStream *stream){
	if(!stream) return 1;
	if(!(stream->cursor_chunk))
		stream->cursor_chunk = stream->chunk_list.head;
	if(!(stream->cursor_chunk)) return 1;
	while(chunkStream_overstepCursorp(stream)){
		if(!(stream->cursor_chunk->next)) return 2;
		stream->cursor_chunk_offset -= ((struct extent*)(stream->cursor_chunk->data))->len;
		stream->cursor_chunk = stream->cursor_chunk->next;
	}
	return 0;
}
int chunkStream_seekForward(struct chunkStream *stream, size_t offset){
	if(chunkStream_reduceCursor(stream)) return 1;
	stream->cursor_chunk_offset += offset;
	return chunkStream_reduceCursor(stream);
}
int chunkStream_takeBytes(struct chunkStream *stream, size_t len, struct extent *result){
	char *ptr;
	size_t length_remaining;
	struct extent *current;
	if(!len){
		result->bytes = 0;
		result->len = 0;
		return 0;
	}
	ptr = palloc(stream->pool, len + 1);
	memset(ptr, 0, len + 1);
	result->bytes = ptr;
	result->len = len;
	if(!(stream->cursor_chunk)) stream->cursor_chunk = stream->chunk_list.head;
	chunkStream_reduceCursor(stream);
	current = (struct extent*)(stream->cursor_chunk->data);
	length_remaining = current->len - stream->cursor_chunk_offset;
	if(len <= length_remaining){
		if(len)
			memcpy(ptr, current->bytes + stream->cursor_chunk_offset, len);
		ptr += len;
		*ptr = 0;
		return chunkStream_seekForward(stream, len);
	}
	if(length_remaining)
		memcpy(ptr, current->bytes + stream->cursor_chunk_offset, length_remaining);
	ptr += length_remaining;
	len -= length_remaining;
	if(chunkStream_seekForward(stream, length_remaining)) return 1;
	current = (struct extent*)(stream->cursor_chunk->data);
	while(len > current->len){
		if(current->len)
			memcpy(ptr, current->bytes, current->len);
		ptr += current->len;
		len -= current->len;
		if(chunkStream_seekForward(stream, length_remaining)) return 2;
		current = (struct extent*)(stream->cursor_chunk->data);
	}
	if(len)
		memcpy(ptr, current->bytes, len);
	ptr += len;
	*ptr = 0;
	if(chunkStream_seekForward(stream, len)) return 3;
	return 0;
}
char chunkStream_byteAtRelativeOffset(struct chunkStream *stream, int offset){
	struct linked_list *cursor;
	if(chunkStream_reduceCursor(stream)) return 0;
	if(!(stream->cursor_chunk)) return 0;
	cursor = stream->cursor_chunk;
	offset += stream->cursor_chunk_offset;
	if(offset < 0) return 0;
	while(cursor){
		if((size_t)offset < ((struct extent*)(cursor->data))->len)
			return ((struct extent*)(cursor->data))->bytes[offset];
		offset -= ((struct extent*)(cursor->data))->len;
		cursor = cursor->next;
	}
	return 0;
}
int chunkStream_findByteOffsetFrom(struct chunkStream *stream, char target, int initial_offset){
	struct chunkStream cursor;
	int result = 0;
	if(!stream) return -1;
	if(chunkStream_reduceCursor(stream)) return -1;
	cursor.pool = stream->pool;
	cursor.chunk_list.head = stream->chunk_list.head;
	cursor.chunk_list.tail = stream->chunk_list.tail;
	cursor.cursor_chunk = stream->cursor_chunk;
	cursor.cursor_chunk_offset = stream->cursor_chunk_offset;
	if(chunkStream_seekForward(&cursor, initial_offset)) return -1;
	while(1){
		if(target == chunkStream_byteAtRelativeOffset(&cursor, 0)) return result;
		if(chunkStream_seekForward(&cursor, 1)) return -1;
		result++;
	}
	return -1;
}


int chunkStream_length(struct chunkStream *stream){
	size_t len = 0;
	if(!stream) return 0;
	if(traverse_linked_list(stream->chunk_list.head, (visitor_t)(&visit_extent_sumLength), &len))
		return -1;
	return len;
}
int chunkStream_lengthRemaining(struct chunkStream *stream){
	size_t len = 0;
	if(!stream) return 0;
	if(!(stream->cursor_chunk)) return 0;
	if(traverse_linked_list(stream->cursor_chunk->next, (visitor_t)(&visit_extent_sumLength), &len))
		return -1;
	return len + ((struct extent*)(stream->cursor_chunk->data))->len - stream->cursor_chunk_offset;
}


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

int requestInput_readChunk(struct requestInput *req, char* buf, int len){
	if(!req) return 1;
	if(!buf) return 1;
	if(len > CHUNK_SIZE) return 1;
	buf[len] = 0;
	chunkStream_append(req->chunks, buf, len);
	if(len) return 0;
	req->done = 1;
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

int requestInput_getBodyLengthSoFar(struct requestInput *req){
	int len = -1;
	int current = 0;
	if(!req) return len;
	if(!(req->headersDone)) return len;
	current = chunkStream_length(req->body);
	if(-1 == current) return current;
	len = current;
	current = chunkStream_lengthRemaining(req->chunks);
	if(-1 == current) return current;
	len += current;
	return len;
}
