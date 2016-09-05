/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>


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

int await_a_resource(int listening_socket, void *connections, struct timeval *timeout, int *out_fd){
	fd_set to_read;
	int status;
	int nconns;
	FD_ZERO(&to_read);
	status = 0;
	nconns = 0 * (long int)connections; /* TODO: this arithmetic suppresses a warning, so get rid of it when the warning stops needing suppression */
	FD_SET(listening_socket, &to_read);
	/*
		TODO:
			loop over connections
				FD_SET each of them
				count them in nconns
	*/
	status = select(nconns + 1, &to_read, 0, 0, timeout);
	if(-1 == status){
		FD_ZERO(&to_read);
		return 2;
	}
	if(!status){
		FD_ZERO(&to_read);
		return 1;
	}
	if(FD_ISSET(listening_socket, &to_read)){
		FD_ZERO(&to_read);
		*out_fd = listening_socket;
		return 0;
	}
	/* TODO: loop over connections and do like above */
	FD_ZERO(&to_read);
	return 3;
}

int accept_new_connection(int server_socket, void *connections){
	struct sockaddr address;
	socklen_t length;
	int fd;
	fd = accept(server_socket, &address, &length);
	if(-1 == fd) return 1;
	/* TODO: append fd to connections */

	printf("new connection on socket with file descriptor %d\n", (int)(fd + 0 * (long int)connections));
	close(fd);

	return 0;
}

int manage_resources_forever(int listening_socket){
	void *connections;
	struct timeval timeout;
	int ready_fd;
	connections = 0;
	while(1){
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if(!await_a_resource(listening_socket, connections, &timeout, &ready_fd)){
			if(ready_fd == listening_socket){
				accept_new_connection(listening_socket, connections);
			}
			else{
			}
		}
	}
	return 0;
}

int main(int argument_count, char* *arguments_vector){
  int sockfd = -1;
	if(2 != argument_count) return 1;
  if(get_socket(arguments_vector[1], &sockfd)) return 2;
	if(listen(sockfd, 32)) return 3;
	accept_new_connection(sockfd, 0);
	if(shutdown(sockfd, SHUT_RDWR)) return 4;
	if(close(sockfd)) return 5;
  return 0;
}
