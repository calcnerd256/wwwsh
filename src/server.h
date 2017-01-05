/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./server_structs.h"

int httpServer_init(struct httpServer *server);

int httpServer_pushResource(
	struct httpServer *server,
	struct linked_list *new_head,
	struct httpResource *resource,
	int (*urlMatchesp)(struct httpResource*, struct extent*),
	int (*respond)(struct httpResource*, struct conn_bundle*),
	void *context
);

int httpServer_listen(struct httpServer *server, char* port_number, int backlog);
int httpServer_close(struct httpServer *server);
int httpServer_selectRead(struct httpServer *server);
int httpServer_acceptNewConnection(struct httpServer *server);
int httpServer_removeEmptyConnections(struct httpServer *server);

int match_httpResource_url(struct httpResource *resource, struct extent *url, struct linked_list *node);

int httpServer_stepConnections(struct httpServer *server);
