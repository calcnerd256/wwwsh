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
	struct mempool *pool;
	int listeningSocket_fileDescriptor;
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
	unsigned long int request_length;
	int fd;
	size_t cursor_chunk_offset;
	int http_major_version;
	int http_minor_version;
	char done_reading;
	char done_writing;
};

int init_connection(struct conn_bundle *ptr, struct mempool *allocations, int fd){
	ptr->pool = allocations;
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
	return 0;
}

#include "./get_socket.c"
#include "./await_a_resource.c"
#include "./accept_new_connection.c"
#include "./handle_chunk.c"
#include "./visit_connection_bundle_process_step.c"

/* #include "./test_pools.c" */


int httpServer_init(struct httpServer *server){
	server->listeningSocket_fileDescriptor = -1;
	server->pool = malloc(sizeof(struct mempool));
	init_pool(server->pool);
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
	if(server->pool){
		free_pool(server->pool);
		free(server->pool);
		server->pool = 0;
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

int main(int argument_count, char* *arguments_vector){
	struct httpServer server;
	struct timeval timeout;
	int ready_fd;
	struct linked_list *connections = 0;
	if(2 != argument_count) return 1;
	if(httpServer_init(&server)) return 1;
	if(httpServer_listen(&server, arguments_vector[1], 32)){
		server.listeningSocket_fileDescriptor = -1;
		httpServer_close(&server);
		return 2;
	}

	while(1){
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if(!await_a_resource(server.listeningSocket_fileDescriptor, &timeout, &ready_fd, connections)){
			if(ready_fd == server.listeningSocket_fileDescriptor){
				accept_new_connection(server.listeningSocket_fileDescriptor, server.pool, &connections);
			}
			else{
				handle_chunk(ready_fd, connections);
			}
		}
		traverse_linked_list(connections, (visitor_t)(&visit_connection_bundle_process_step), 0);
	}

	if(httpServer_close(&server)){
		server.listeningSocket_fileDescriptor = -1;
		return 4;
	}
	return 0;
}
