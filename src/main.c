/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>


struct fd_list{
	int fd;
	struct fd_list *next;
};

int free_fd_list_closing(struct fd_list *head){
	struct fd_list *ptr;
	while(head){
		close(head->fd);
		ptr = head->next;
		head->fd = -1;
		head->next = 0;
		free(head);
		head = ptr;
	}
	return 0;
}

struct fd_list *last(struct fd_list *head){
	struct fd_list *ptr;
	int counter;
	if(!head) return head;
	ptr = 0;
	counter = 0;
	while(head->next){
		if(ptr == head) return head;
		if(!ptr) ptr = head;
		head = head->next;
		if(counter) ptr = ptr->next;
		counter = !counter;
	}
	return head;
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

int await_a_resource(int listening_socket, struct fd_list *connections, struct timeval *timeout, int *out_fd){
	fd_set to_read;
	int status;
	int maxfd;
	struct fd_list *ptr;
	FD_ZERO(&to_read);
	status = 0;
	maxfd = listening_socket;
	FD_SET(listening_socket, &to_read);
	ptr = connections->next;
	while(ptr){
		/* TODO: cycle detection */
		FD_SET(ptr->fd, &to_read);
		if(maxfd < ptr->fd) maxfd = ptr->fd;
		ptr = ptr->next;
	}
	status = select(maxfd + 1, &to_read, 0, 0, timeout);
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
	ptr = connections->next;
	while(ptr){
		/* TODO: cycle detection */
		if(FD_ISSET(ptr->fd, &to_read)){
			FD_ZERO(&to_read);
			*out_fd = ptr->fd;
			return 0;
		}
		ptr = ptr->next;
	}
	FD_ZERO(&to_read);
	return 3;
}

int accept_new_connection(int server_socket, struct fd_list *connections){
	struct sockaddr address;
	socklen_t length;
	int fd;
	struct fd_list *new_last;
	memset(&address, 0, sizeof address);
	length = 0;
	fd = accept(server_socket, &address, &length);
	if(-1 == fd) return 1;
	connections = last(connections);
	if(!connections){
		close(fd);
		return 1;
	}
	new_last = malloc(sizeof(struct fd_list));
	new_last->fd = fd;
	new_last->next = 0;
	connections->next = new_last;
	return 0;
}

int handle_chunk(int sockfd, struct fd_list *connections){
	char buf[256+1];
	size_t len;
	while(connections)
		if(sockfd == connections->fd)
			break;
	if(!connections) return 1;
	buf[256] = 0;
	len = read(sockfd, buf, 256);
	buf[len] = 0;
	printf("read <<<%s>>>\n", buf);
	return 0;
}

int manage_resources_forever(int listening_socket){
	struct fd_list *connections;
	struct timeval timeout;
	int ready_fd;
	connections = malloc(sizeof(struct fd_list));
	connections->fd = listening_socket;
	connections->next = 0;
	while(1){
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if(!await_a_resource(listening_socket, connections, &timeout, &ready_fd)){
			if(ready_fd == listening_socket){
				accept_new_connection(listening_socket, connections);
			}
			else{
				handle_chunk(ready_fd, connections->next);
			}
		}
	}
	free_fd_list_closing(connections->next);
	connections->fd = -1;
	connections->next = 0;
	free(connections);
	return 0;
}

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
