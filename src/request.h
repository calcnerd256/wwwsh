/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./server_structs.h"

/* TODO: rename connection_bundle and struct conn_bundle to incomingHttpRequest */

int init_connection(struct conn_bundle *ptr, struct httpServer *server, int fd);
int incomingHttpRequest_selectRead(struct conn_bundle *req);
int match_by_sockfd(struct conn_bundle *data, int *target, struct linked_list *node);
int httpRequestHandler_readChunk(struct conn_bundle *conn);

int connection_bundle_can_respondp(struct conn_bundle *conn);

int connection_bundle_write_extent(struct conn_bundle *conn, struct extent *str);
int connection_bundle_write_crlf(struct conn_bundle *conn);
int connection_bundle_write_status_line(struct conn_bundle *conn, int status_code, struct extent *reason);
int connection_bundle_write_header(struct conn_bundle *conn, struct extent *key, struct extent *value);

int connection_bundle_free(struct conn_bundle *conn);
int connection_bundle_close_write(struct conn_bundle *conn);

int connection_bundle_send_response(struct conn_bundle *conn, int status_code, struct extent *reason, struct linked_list *headers, struct extent *body);

int connection_bundle_respond_bad_request_target(struct conn_bundle *conn);


int httpResource_respond(struct httpResource *resource, struct conn_bundle *connection);

int connection_bundle_respond(struct conn_bundle *conn);

int connection_bundle_process_steppedp(struct conn_bundle *conn);


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
