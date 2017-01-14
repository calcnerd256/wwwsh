/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./server_structs.h"
#include "./process.h"

int httpServer_init(struct httpServer *server);

int httpServer_pushResource(
	struct httpServer *server,
	struct linked_list *new_head,
	struct httpResource *resource,
	int (*urlMatchesp)(struct httpResource*, struct extent*),
	int (*canRespondp)(struct httpResource*, struct incomingHttpRequest*),
	int (*respond)(struct httpResource*, struct incomingHttpRequest*),
	void *context
);
int httpServer_pushChildProcess(struct httpServer *server, struct childProcessResource *kid);

int httpServer_listen(struct httpServer *server, char* port_number, int backlog);
int httpServer_close(struct httpServer *server);
int httpServer_acceptNewConnection(struct httpServer *server);

struct httpResource* httpServer_locateResource(struct httpServer *server, struct extent *url);

int httpServer_stepConnections(struct httpServer *server);
