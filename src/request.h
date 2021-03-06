/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./server_structs.h"

int incomingHttpRequest_init(struct incomingHttpRequest *ptr, struct httpServer *server, int fd);
int fd_canReadp(int fd);
int match_incomingHttpRequest_bySocketFileDescriptor(struct incomingHttpRequest *data, int *target, struct linked_list *node);


int incomingHttpRequest_beginChunkedResponse(struct incomingHttpRequest *req, int status_code, struct extent *reason, struct linked_list *headers);
int incomingHttpRequest_beginChunkedHtmlOk(struct incomingHttpRequest *req, struct linked_list *headers);

int incomingHttpRequest_write_chunk(struct incomingHttpRequest *req, char* bytes, size_t len);
int incomingHttpRequest_writeChunk_niceString(struct incomingHttpRequest *req, char* str);
int incomingHttpRequest_writelnChunk_niceString(struct incomingHttpRequest *req, char* str);
int incomingHttpRequest_writeChunk_htmlSafeExtent(struct incomingHttpRequest *req, struct extent *str);

int incomingHttpRequest_sendLastChunk(struct incomingHttpRequest *req, struct linked_list *trailers);

int incomingHttpRequest_beginChunkedHtmlBody(struct incomingHttpRequest *req, struct linked_list *headers, char *title, size_t titleLen);
int incomingHttpRequest_endChunkedHtmlBody(struct incomingHttpRequest *req, struct linked_list *trailers);


int incomingHttpRequest_sendResponse(
	struct incomingHttpRequest *conn,
	int status_code,
	struct extent *reason,
	struct linked_list *headers,
	struct extent *body
);

int incomingHttpRequest_respond_badMethod(struct incomingHttpRequest *conn);
int incomingHttpRequest_respond_htmlOk(
	struct incomingHttpRequest *conn,
	struct linked_list *headers,
	struct extent *body
);

int incomingHttpRequest_processSteppedp(struct incomingHttpRequest *conn);


struct linked_list *push_header_nice_strings(
	struct linked_list *top,
	struct linked_list *key_node,
	struct linked_list *value_node,
	struct extent *key_extent,
	char *key,
	struct extent *value_extent,
	char *value,
	struct linked_list *next
);

int extent_url_equalsp(struct extent *target, struct extent *url);
