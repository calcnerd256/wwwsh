/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "./mempool.h"

#define CHUNK_SIZE 256

struct httpServer{
	struct mempool *memoryPool;
	int listeningSocket_fileDescriptor;
	struct linked_list *connections;
};

struct conn_bundle{
	struct mempool *pool;
	struct linked_list *chunks;
	struct linked_list *last_chunk;
	struct linked_list *cursor_chunk;
	struct linked_list *lines;
	struct linked_list *last_line;
	struct extent *method;
	struct extent *request_url;
	struct httpServer *server;
	unsigned long int request_length;
	int fd;
	size_t cursor_chunk_offset;
	int http_major_version;
	int http_minor_version;
	char done_reading;
	char done_writing;
};

int init_connection(struct conn_bundle *ptr, struct httpServer *server, int fd){
	ptr->pool = server->memoryPool;
	ptr->fd = fd;
	ptr->done_reading = 0;
	ptr->chunks = 0;
	ptr->request_length = 0;
	ptr->last_chunk = 0;
	ptr->cursor_chunk = 0;
	ptr->cursor_chunk_offset = 0;
	ptr->lines = 0;
	ptr->last_line = 0;
	ptr->method = 0;
	ptr->request_url = 0;
	ptr->http_major_version = -1;
	ptr->http_minor_version = -1;
	ptr->done_writing = 0;
	ptr->server = server;
	return 0;
}

#include "./get_socket.c"
#include "./visit_connection_bundle_process_step.c"

/* #include "./test_pools.c" */


int httpServer_init(struct httpServer *server){
	server->listeningSocket_fileDescriptor = -1;
	server->memoryPool = malloc(sizeof(struct mempool));
	init_pool(server->memoryPool);
	server->connections = 0;
	server = 0;
	return 0;
}

int httpServer_listen(struct httpServer *server, char* port_number, int backlog){
	int sockfd = -1;
	if(get_socket(port_number, &sockfd)){
		server = 0;
		port_number = 0;
		backlog = 0;
		sockfd = 0;
		return 1;
	}
	port_number = 0;
	server->listeningSocket_fileDescriptor = sockfd;
	server = 0;
	if(listen(sockfd, backlog)){
		backlog = 0;
		sockfd = 0;
		return 2;
	}
	sockfd = 0;
	backlog = 0;
	return 0;
}

int httpServer_close(struct httpServer *server){
	if(server->memoryPool){
		free_pool(server->memoryPool);
		free(server->memoryPool);
		server->memoryPool = 0;
	}
	if(-1 == server->listeningSocket_fileDescriptor){
		server = 0;
		return 0;
	}
	if(shutdown(server->listeningSocket_fileDescriptor, SHUT_RDWR)){
		server = 0;
		return 1;
	}
	if(close(server->listeningSocket_fileDescriptor)){
		server = 0;
		return 2;
	}
	server->listeningSocket_fileDescriptor = -1;
	server = 0;
	return 0;
}

int visit_connection_bundle_select_read(struct conn_bundle *conn, struct linked_list *context, struct linked_list *node){
	int *done;
	int *fd;
	struct timeval timeout;
	fd_set to_read;
	int status;
	(void)node;
	if(!context) return 1;
	if(!context->next) return 1;
	done = (int *)(context->data);
	if(!done) return 3;
	if(*done) return 0;
	fd = (int *)(context->next->data);
	if(!fd) return 3;
	if(!conn) return 2;
	if(conn->done_reading) return 0;
	if(-1 == conn->fd) return 2;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	FD_ZERO(&to_read);
	FD_SET(conn->fd, &to_read);
	status = select(conn->fd + 1, &to_read, 0, 0, &timeout);
	if(-1 == status) return 4;
	if(!status) return 0;
	if(FD_ISSET(conn->fd, &to_read)){
		*fd = conn->fd;
		*done = 1;
	}
	return 0;
}

int httpServer_selectRead(struct httpServer *server){
	int ready_fd;
	struct linked_list context;
	struct linked_list fd_cell;
	struct conn_bundle fake_for_server;
	struct linked_list extra_head;
	int done;
	done = 0;
	ready_fd = -1;
	fd_cell.next = 0;
	fd_cell.data = &ready_fd;
	context.next = &fd_cell;
	context.data = &done;
	extra_head.next = server->connections;
	extra_head.data = &fake_for_server;
	fake_for_server.fd = server->listeningSocket_fileDescriptor;
	fake_for_server.done_reading = 0;
	if(traverse_linked_list(&extra_head, (visitor_t)(&visit_connection_bundle_select_read), &context)) return -1;
	return ready_fd;
}

int httpServer_stepConnections(struct httpServer *server){
	int any = 0;
	if(traverse_linked_list(server->connections, (visitor_t)(&visit_connection_bundle_process_step), &any)) return 0;
	return any;
}

int httpServer_acceptNewConnection(struct httpServer *server){
	struct sockaddr address;
	socklen_t length;
	int fd;
	char *ptr;
	struct linked_list *new_head;

	memset(&address, 0, sizeof(struct sockaddr));
	length = 0;
	fd = accept(server->listeningSocket_fileDescriptor, &address, &length);
	if(-1 == fd) return 1;

	ptr = palloc(server->memoryPool, sizeof(struct conn_bundle) + sizeof(struct linked_list));
	new_head = (struct linked_list*)ptr;
	new_head->next = server->connections;
	server->connections = new_head;
	new_head->data = ptr + sizeof(struct linked_list);
	init_connection((struct conn_bundle*)(new_head->data), server, fd);
	return 0;
}

int match_by_sockfd(struct conn_bundle *data, int *target, struct linked_list *node){
	(void)node;
	return data->fd == *target;
}

int httpRequestHandler_readChunk(struct conn_bundle *conn){
	/*

	find the connection attached to the socket
	contiguously allocate the extent for the chunk, the list node pointing to it, and the buffer of its bytes
	read the bytes into the buffer, point the extent at it, and append this to the connection's chunk list
	if the chunk list is empty, appending is assignment of the node to the list's head
	if the read is empty, the socket has been closed, so close the read end of the connection

	*/

	char *buf;
	size_t len;
	struct extent *chunkptr;
	struct linked_list *new_head;

	if(!conn) return 2;

	buf = palloc(conn->pool, CHUNK_SIZE + 1 + sizeof(struct extent) + sizeof(struct linked_list));
	chunkptr = (struct extent*)(buf + CHUNK_SIZE + 1);
	new_head = (struct linked_list*)(buf + CHUNK_SIZE + 1 + sizeof(struct extent));
	new_head->next = 0;
	buf[CHUNK_SIZE] = 0;
	len = read(conn->fd, buf, CHUNK_SIZE);
	buf[len] = 0;
	chunkptr->bytes = buf;
	chunkptr->len = len;
	new_head->data = chunkptr;
	*(conn->last_chunk ? &(conn->last_chunk->next) : &(conn->chunks)) = new_head;
	conn->last_chunk = new_head;
	conn->request_length += len;

	if(len) return 0;
	conn->done_reading = 1;
	if(conn->done_writing)
		close(conn->fd);
	return 0;
}

int main(int argument_count, char* *arguments_vector){
	struct httpServer server;
	int ready_fd;
	struct linked_list *match_node;
	if(2 != argument_count) return 1;
	if(httpServer_init(&server)) return 1;
	if(httpServer_listen(&server, arguments_vector[1], 32)){
		server.listeningSocket_fileDescriptor = -1;
		httpServer_close(&server);
		return 2;
	}

	while(1){
		ready_fd = httpServer_selectRead(&server);
		if(-1 != ready_fd){
			if(ready_fd == server.listeningSocket_fileDescriptor)
				httpServer_acceptNewConnection(&server);
			else{
				match_node = 0;
				if(!first_matching(server.connections, (visitor_t)(&match_by_sockfd), (struct linked_list *)(&ready_fd), &match_node))
					if(match_node)
						httpRequestHandler_readChunk(match_node->data);
			}
		}
		if(!httpServer_stepConnections(&server))
			if(ready_fd == -1)
				usleep(10);
	}

	if(httpServer_close(&server)){
		server.listeningSocket_fileDescriptor = -1;
		return 4;
	}
	return 0;
}
