/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./mempool.h"

#define CHUNK_SIZE 256


struct chunkStream{
	struct mempool *pool;
	struct linked_list *chunks;
	struct linked_list *last_chunk;
	struct linked_list *cursor_chunk;
	size_t cursor_chunk_offset;
};

int chunkStream_init(struct chunkStream *ptr, struct mempool *pool);
int chunkStream_append(struct chunkStream *stream, char *bytes, size_t len);
int chunkStream_overstepCursorp(struct chunkStream *stream);
int chunkStream_reduceCursor(struct chunkStream *stream);
int chunkStream_seekForward(struct chunkStream *stream, size_t offset);
int chunkStream_takeBytes(struct chunkStream *stream, size_t len, struct extent *result);
char chunkStream_byteAtRelativeOffset(struct chunkStream *stream, int offset);
int chunkStream_findByteOffsetFrom(struct chunkStream *stream, char target, int initial_offset);
