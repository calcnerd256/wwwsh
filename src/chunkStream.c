/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <string.h>
#include "./chunkStream.h"

int chunkStream_init(struct chunkStream *ptr, struct mempool *pool){
	ptr->pool = pool;
	ptr->chunks = 0;
	ptr->last_chunk = 0;
	ptr->cursor_chunk = 0;
	ptr->cursor_chunk_offset = 0;
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
	bytes = bufptr;
	bufptr += len;
	*bufptr = 0;
	bufptr = 0;
	new_head->next = 0;
	chunk->len = len;
	len = 0;
	chunk->bytes = bytes;
	bytes = 0;
	new_head->data = chunk;
	chunk = 0;
	if(!stream->last_chunk) stream->last_chunk = stream->chunks;
	if(!stream->last_chunk) stream->chunks = new_head;
	else stream->last_chunk->next = new_head;
	stream->last_chunk = new_head;
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
		stream->cursor_chunk = stream->chunks;
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
	ptr = palloc(stream->pool, len + 1);
	memset(ptr, 0, len + 1);
	result->bytes = ptr;
	result->len = len;
	if(!(stream->cursor_chunk)) stream->cursor_chunk = stream->chunks;
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
