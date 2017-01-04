/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <unistd.h>
#include <string.h>
#include "./request.h"
#include "./server.h"

/* TODO: rename move this method as appropriate */
int init_connection(struct conn_bundle *ptr, struct httpServer *server, int fd){
	memset(&(ptr->allocations), 0, sizeof(struct mempool));
	init_pool(&(ptr->allocations));
	ptr->fd = fd;
	fd = 0;
	ptr->done_writing = 0;
	ptr->server = server;
	server = 0;
	requestInput_init(&(ptr->input), &(ptr->allocations));
	ptr->input.fd = ptr->fd;
	ptr->node = 0;
	ptr = 0;
	return 0;
}

int incomingHttpRequest_selectRead(struct conn_bundle *req){
	struct timeval timeout;
	fd_set to_read;
	int status;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	FD_ZERO(&to_read);
	if(!req) return -1;
	if(req->input.done) return -1;
	if(-1 == req->fd) return -1;
	FD_SET(req->fd, &to_read);
	status = select(req->fd + 1, &to_read, 0, 0, &timeout);
	if(-1 == status) return -1;
	if(!status) return -1;
	if(!FD_ISSET(req->fd, &to_read)) return -1;
	FD_ZERO(&to_read);
	return req->fd;
}

int connection_bundle_can_respondp(struct conn_bundle *conn){
	if(!(conn->input.method)) return 0;
	if(!(conn->input.requestUrl)) return 0;
	if(-1 == conn->input.httpMajorVersion) return 0;
	if(-1 == conn->input.httpMinorVersion) return 0;
	if(conn->done_writing) return 0;
	return 1;
}


int connection_bundle_write_extent(struct conn_bundle *conn, struct extent *str){
	ssize_t bytes;
	bytes = write(conn->fd, str->bytes, str->len);
	if(bytes < 0) return 1;
	if((size_t)bytes != str->len) return 2;
	return 0;
}
