/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>


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

int main(int argument_count, char* *arguments_vector){
  int sockfd = -1;
	if(2 != argument_count) return 1;
  if(get_socket(arguments_vector[1], &sockfd)) return 2;
	if(listen(sockfd, 32)) return 5;
	if(shutdown(sockfd, SHUT_RDWR)) return 3;
	if(close(sockfd)) return 4;
  return 0;
}
