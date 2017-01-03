/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./chunkStream.h"

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
