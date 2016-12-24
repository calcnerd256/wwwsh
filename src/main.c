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

int manage_resources_forever(int listening_socket){
	struct timeval timeout;
	int ready_fd;
	struct mempool pool;
	struct linked_list *connections = 0;
	init_pool(&pool);
	while(1){
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if(!await_a_resource(listening_socket, &timeout, &ready_fd, connections)){
			if(ready_fd == listening_socket){
				accept_new_connection(listening_socket, &pool, &connections);
			}
			else{
				handle_chunk(ready_fd, connections);
			}
		}
		traverse_linked_list(connections, (visitor_t)(&visit_connection_bundle_process_step), 0);
	}
	free_pool(&pool);
	return 0;
}

/* #include "./test_pools.c" */


int main(int argument_count, char* *arguments_vector){
	int sockfd;
	sockfd = -1;
	if(2 != argument_count) return 1;
	if(get_socket(arguments_vector[1], &sockfd)) return 2;
	if(listen(sockfd, 32)) return 3;

	manage_resources_forever(sockfd);

	if(shutdown(sockfd, SHUT_RDWR)) return 4;
	if(close(sockfd)) return 5;
	return 0;
}
