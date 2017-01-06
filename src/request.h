/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./server_structs.h"

int incomingHttpRequest_init(struct incomingHttpRequest *ptr, struct httpServer *server, int fd);
int incomingHttpRequest_selectRead(struct incomingHttpRequest *req);
int match_incomingHttpRequest_bySocketFileDescriptor(struct incomingHttpRequest *data, int *target, struct linked_list *node);

int incomingHttpRequest_write_crlf(struct incomingHttpRequest *conn);
int incomingHttpRequest_write_chunk(struct incomingHttpRequest *req, char* bytes, size_t len);
int incomingHttpRequest_closeWrite(struct incomingHttpRequest *conn);
int incomingHttpRequest_sendResponseHeaders(struct incomingHttpRequest *conn, int status_code, struct extent *reason, struct linked_list *headers);


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
