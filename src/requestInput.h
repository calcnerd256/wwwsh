/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./mempool.h"


#define CHUNK_SIZE 256

struct chunkStream{
	struct dequoid chunk_list;
	struct mempool *pool;
	struct linked_list *cursor_chunk;
	size_t cursor_chunk_offset;
};

int chunkStream_init(struct chunkStream *ptr, struct mempool *pool);
int chunkStream_append(struct chunkStream *stream, char *bytes, size_t len);
int chunkStream_reduceCursor(struct chunkStream *stream);
int chunkStream_seekForward(struct chunkStream *stream, size_t offset);
int chunkStream_takeBytes(struct chunkStream *stream, size_t len, struct extent *result);
char chunkStream_byteAtRelativeOffset(struct chunkStream *stream, int offset);
int chunkStream_findByteOffsetFrom(struct chunkStream *stream, char target, int initial_offset);

int chunkStream_length(struct chunkStream *stream);
int chunkStream_lengthRemaining(struct chunkStream *stream);


struct requestInput{
	struct mempool *pool;
	struct chunkStream *chunks;
	struct extent *method;
	struct extent *requestUrl;
	struct dequoid *headers;
	struct chunkStream *body;
	unsigned long int length;
	int fd;
	int httpMajorVersion;
	int httpMinorVersion;
	char done;
	char headersDone;
};

int requestInput_init(struct requestInput *req, struct mempool *pool);
int requestInput_readChunk(struct requestInput *req, char* buf, int len);
int requestInput_findCrlfOffset(struct requestInput *req);
int requestInput_printHeaders(struct requestInput *req);
int requestInput_consumeHeader(struct requestInput *req);
int requestInput_consumeLine(struct requestInput *req);
int requestInput_consumeLastLine(struct requestInput *req);
int requestInput_consumeMethod(struct requestInput *req);
int requestInput_consumeRequestUrl(struct requestInput *req);
int requestInput_consumeHttpVersion(struct requestInput *req);
int requestInput_processStep(struct requestInput *req);
int requestInput_printBody(struct requestInput *req);
int requestInput_getBodyLengthSoFar(struct requestInput *req);
