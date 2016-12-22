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

int get_socket(char* port_number, int *out_sockfd){
	struct addrinfo *server_address;
	struct addrinfo hint;

	server_address = 0;
	memset(&hint, 0, sizeof hint);

	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags = AI_PASSIVE;

	if(getaddrinfo(0, port_number, &hint, &server_address)){
		if(server_address){
			freeaddrinfo(server_address);
			server_address = 0;
		}
		return 2;
	}

  *out_sockfd = socket(server_address->ai_family, server_address->ai_socktype, server_address->ai_protocol);
  if(-1 == *out_sockfd){
		freeaddrinfo(server_address);
		server_address = 0;
		return 3;
	}

	if(bind(*out_sockfd, server_address->ai_addr, server_address->ai_addrlen)){
		freeaddrinfo(server_address);
		server_address = 0;
		shutdown(*out_sockfd, SHUT_RDWR);
		close(*out_sockfd);
		return 4;
	}

	freeaddrinfo(server_address);
	server_address = 0;
	return 0;
}

#include "./manage_resources_forever.c"

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
