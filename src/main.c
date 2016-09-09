/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "./linked_list.h"


struct connection_state{
	struct linked_list *request;
	int fd;
	char done_reading;
};

int init_connection_state(struct connection_state *ptr){
	ptr->fd = 0;
	ptr->request = malloc(sizeof(struct linked_list));
	ptr->request->data = 0;
	ptr->request->next = 0;
	ptr->done_reading = 0;
	return 0;
}

int visit_connection_state_print_fd(struct connection_state *state, void *context, struct linked_list *node){
	if(!state){
		printf("null state\n");
		return (int)(0 * (long)context);
	}
	printf("fd %d\n", state->fd);
	return (int)(0 * (long)node);
}
int print_connection_state_list(struct linked_list *connection_list){
	printf("connection states:\n");
	traverse_linked_list(connection_list, (visitor_t)(&visit_connection_state_print_fd), 0);
	printf("\n");
	return 0;
}

int visit_free_string(char *buf, void *context, struct linked_list *node){
	free(buf);
	return (int)(0 * (long)(context) * (long)(node));
}
int clean_connection(struct connection_state *state, void *context, struct linked_list *node){
	close(state->fd);
	state->fd = 0;
	clean_and_free_list(state->request->next, (visitor_t)(&visit_free_string), context);
	state->request->next = 0;
	state->request = 0;
	state->done_reading = 1;
	return (int)(0 * (long)node);
}
int free_connection_list(struct linked_list *head){
	return clean_and_free_list(head, (visitor_t)(&clean_connection), 0);
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

int visit_set_fds_and_max(struct connection_state *conn, struct linked_list *context, struct linked_list *node){
	fd_set *to_read;
	int *maxfd;
	if(conn->done_reading) return 0;
	to_read = (fd_set*)(context->data);
	maxfd = (int*)(context->next->data);
  FD_SET(conn->fd, to_read);
	if(*maxfd < conn->fd)
		*maxfd = conn->fd;
	return (int)(0 * (long)node);
}

int visit_fds_isset_status(struct connection_state *conn, struct linked_list *context, struct linked_list *node){
	fd_set *to_read;
	int *out_fd;
	int *found;
	to_read = (fd_set*)(context->data);
	out_fd = (int*)(context->next->data);
	found = (int*)(context->next->next->data);
	if(*found) return (int)(0 * (long)node);
	if(!FD_ISSET(conn->fd, to_read)) return 0;
	*found = 1;
	*out_fd = conn->fd;
	return 0;
}

int await_a_resource(int listening_socket, struct linked_list *connection_list, struct timeval *timeout, int *out_fd){
	fd_set to_read;
	int status;
	int maxfd;
	struct linked_list context[3];
	FD_ZERO(&to_read);
	status = 0;
	maxfd = listening_socket;
	FD_SET(listening_socket, &to_read);

	context[0].data = (void*)&to_read;
	context[0].next = &(context[1]);
	context[1].data = (void*)&maxfd;
	context[1].next = &(context[2]);
	context[2].data = 0;
	context[2].next = 0;
	traverse_linked_list(connection_list->next, (visitor_t)(&visit_set_fds_and_max), context);

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

	context[1].data = (void*)out_fd;
	context[2].data = (void*)&status;
	status = 0;
	traverse_linked_list(connection_list->next, (visitor_t)(&visit_fds_isset_status), context);
	if(status) return 0;

	FD_ZERO(&to_read);
	return 3;
}

int accept_new_connection(int server_socket, struct linked_list *connection_list){
	struct sockaddr address;
	socklen_t length;
	int fd;
	struct connection_state *new_state;
	memset(&address, 0, sizeof address);
	length = 0;
	fd = accept(server_socket, &address, &length);
	if(-1 == fd) return 1;
	new_state = malloc(sizeof(struct connection_state));
	init_connection_state(new_state);
	new_state->fd = fd;
	return append(connection_list, new_state);
}

int match_by_sockfd(struct connection_state *data, int *target, struct linked_list *node){
	if(node){}
	return data->fd == *target;
}

int visit_print_string(char *buf, void *context, struct linked_list *node){
	printf("%s", buf);
	return (int)(0 * (long)context * (long)node);
}

int handle_chunk(int sockfd, struct linked_list *connection_list){
	char *buf;
	size_t len;
	struct linked_list *match_node;
	if(first_matching(connection_list, (visitor_t)(&match_by_sockfd), (struct linked_list*)(&sockfd), &match_node)) return 1;
	buf = malloc(256+1);
	buf[256] = 0;
	len = read(sockfd, buf, 256);
	buf[len] = 0;
	if(!len){
		((struct connection_state*)(match_node->data))->done_reading = 1;
		printf("request socket with file descriptor %d closed; body follows:\n", ((struct connection_state*)(match_node->data))->fd);
		traverse_linked_list(((struct connection_state*)(match_node->data))->request->next, (visitor_t)(&visit_print_string), 0);
		printf("\n\nDone.\n");
		return 0;
	}
	append(((struct connection_state*)(match_node->data))->request, (void*)buf);
	printf("read %d bytes\n", (int)len);
	return 0;
}

int manage_resources_forever(int listening_socket){
	struct linked_list *connection_list;
	struct timeval timeout;
	int ready_fd;
	connection_list = malloc(sizeof(struct linked_list));
	connection_list->data = 0;
	connection_list->next = 0;
	while(1){
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if(!await_a_resource(listening_socket, connection_list, &timeout, &ready_fd)){
			if(ready_fd == listening_socket){
				accept_new_connection(listening_socket, connection_list);
			}
			else{
				handle_chunk(ready_fd, connection_list->next);
			}
		}
	}
	free_connection_list(connection_list->next);
	connection_list->next = 0;
	free(connection_list);
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
