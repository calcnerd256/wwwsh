/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

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
