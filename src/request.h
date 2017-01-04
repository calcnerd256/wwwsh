/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./server_structs.h"

/* TODO: rename connection_bundle and struct conn_bundle to incomingHttpRequest */

int init_connection(struct conn_bundle *ptr, struct httpServer *server, int fd);
