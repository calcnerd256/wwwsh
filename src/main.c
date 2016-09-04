/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

struct addrinfo *server_address;
int cleanup(int status){
	if(server_address){
		freeaddrinfo(server_address);
		server_address = 0;
	}
	return status;
}

int main(int argument_count, char* *arguments_vector){
	struct addrinfo hint;
	char ipv4[INET_ADDRSTRLEN];
	server_address = 0;
	if(2 != argument_count) return cleanup(1);
	memset(&hint, 0, sizeof hint);
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags = AI_PASSIVE;
	if(getaddrinfo(0, arguments_vector[1], &hint, &server_address)) return cleanup(2);
	printf("%x %s %s %s %d %p <%s> %p\n", server_address->ai_flags,
		server_address->ai_family == AF_INET ? "AF_INET" : "other?",
		server_address->ai_socktype == SOCK_STREAM ? "SOCK_STREAM" : "other?",
		getprotobynumber(server_address->ai_protocol)->p_name,
		server_address->ai_addrlen,
		server_address->ai_addr,
		server_address->ai_canonname,
		server_address->ai_next
	);
  if(AF_INET == server_address->ai_family){
		inet_ntop(AF_INET, &(((struct sockaddr_in*)(server_address->ai_addr))->sin_addr), ipv4, INET_ADDRSTRLEN);
		printf("%s:%d\n",
			ipv4,
			ntohs(((struct sockaddr_in*)(server_address->ai_addr))->sin_port)
		);
	}
	return cleanup(0);
}
