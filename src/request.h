/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./server_structs.h"

/* TODO: rename connection_bundle to incomingHttpRequest */

int incomingHttpRequest_init(struct incomingHttpRequest *ptr, struct httpServer *server, int fd);
int incomingHttpRequest_selectRead(struct incomingHttpRequest *req);
int match_incomingHttpRequest_bySocketFileDescriptor(struct incomingHttpRequest *data, int *target, struct linked_list *node);

int connection_bundle_send_response(struct incomingHttpRequest *conn, int status_code, struct extent *reason, struct linked_list *headers, struct extent *body);

int connection_bundle_respond_bad_method(struct incomingHttpRequest *conn);

int connection_bundle_respond(struct incomingHttpRequest *conn);

int connection_bundle_process_steppedp(struct incomingHttpRequest *conn);


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
