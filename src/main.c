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

struct linked_list{
	void *data;
	struct linked_list *next;
};

struct connection_state{
	int fd;
};

struct fd_list{
	int fd;
	struct fd_list *next;
};

typedef int (*visitor_t)(void*, void*, struct linked_list*);

char *alloc_copy_visitor(visitor_t visitor){
	char *result;
	result = malloc(sizeof(visitor_t));
	memcpy(result, &visitor, sizeof(visitor_t));
	return result;
}
int free_visitor_copy(char *bytes){
	memset(bytes, 0, sizeof(visitor_t));
	free(bytes);
	return 0;
}
int apply_visitor(char *fnptr_bytes, void* data, void* context, struct linked_list *node){
	visitor_t visitor;
	memcpy(&visitor, fnptr_bytes, sizeof(visitor_t));
	return (*visitor)(data, context, node);
}

int traverse_linked_list(struct linked_list *head, visitor_t visitor, void *context){
	int status;
	struct linked_list *cycle_detector;
	char counter;
	if(!head) return 0;
	cycle_detector = 0;
	counter = 0;
	while(head){
		if(cycle_detector == head) return 0;
		status = (*visitor)(head->data, context, head);
		if(status) return status;
		if(!cycle_detector) cycle_detector = head;
		head = head->next;
		if(counter) cycle_detector = cycle_detector->next;
		counter = !counter;
	}
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

int clean_and_free_list(struct linked_list *head, visitor_t cleaner, void *context){
	int status;
	struct linked_list *ptr;
	status = traverse_linked_list(head, cleaner, context);
	if(status) return status;
	while(head){
		head->data = 0;
		ptr = head->next;
		head->next = 0;
		free(head);
		head = ptr;
	}
	return 0;
}

int clean_connection(struct connection_state *state, void *context, struct linked_list *head){
	close(state->fd);
	return (int)(0 * ((long)context + (long)head));
}
int free_connection_list(struct linked_list *head){
	return clean_and_free_list(head, (visitor_t)(&clean_connection), 0);
}

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

int visit_matcher(void *data, struct linked_list *context, struct linked_list *node){
	char *matcher;
	struct linked_list* *result;
	matcher = (char*)(context->data);
	result = (struct linked_list**)(context->next->data);
	if(!apply_visitor(matcher, data, context->next->next, node)) return 0;
	*result = node;
	return 1;
}
int first_matching(struct linked_list *head, visitor_t matcher, struct linked_list *context, struct linked_list* *out_match){
	struct linked_list outer_context[3];
	int result;
	outer_context[0].data = alloc_copy_visitor(matcher);
	outer_context[0].next = &(outer_context[1]);
	outer_context[1].data = (void*)out_match;
	outer_context[1].next = context;
	result = !(traverse_linked_list(head, (visitor_t)(&visit_matcher), outer_context));
	free_visitor_copy(outer_context[0].data);
	return result;
}

int match_last(void *data, void *context, struct linked_list *node){
	return !(node->next) + (int)(0 * (long)data * (long)context);
}
/*
int visitor_last(void *data, struct linked_list* *context, struct linked_list *head){
	*context = head;
	return (int)(0 * (long)data);
}
*/
struct linked_list *last_node(struct linked_list *head){
	struct linked_list *result = 0;
	if(first_matching(head, match_last, 0, &result)) return 0;
	/*traverse_linked_list(head, (visitor_t)(&visitor_last), (struct linked_list*)(&result));*/
	return result;
}

int append(struct linked_list *head, void* data){
	head = last_node(head);
	head->next = malloc(sizeof(struct linked_list));
	head = head->next;
	head->data = data;
	head->next = 0;
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

int visit_set_fds_and_max(struct connection_state *conn, struct linked_list *context, struct linked_list *node){
	fd_set *to_read;
	int *maxfd;
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

int await_a_resource(int listening_socket, struct fd_list *connections, struct timeval *timeout, int *out_fd){
	fd_set to_read;
	int status;
	int maxfd;
	struct fd_list *ptr;
	struct linked_list context[3];
	FD_ZERO(&to_read);
	status = 0;
	maxfd = listening_socket;
	FD_SET(listening_socket, &to_read);
	ptr = connections->next;
	context[0].data = (void*)&to_read;
	context[0].next = &(context[1]);
	context[1].data = (void*)&maxfd;
	context[1].next = &(context[2]);
	context[2].data = 0;
	context[2].next = 0;
	/*traverse_linked_list(connections->next, (visitor_t)(&visit_set_fds_and_max), context);*/
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

	context[1].data = (void*)out_fd;
	context[2].data = (void*)&status;
	status = 0;
	/*traverse_linked_list(connections->next, (visitor_t)(&visit_fds_isset_status), context);*/
	if(status) return 0;
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

int accept_new_connection(int server_socket, struct fd_list *connections, struct linked_list *connection_list){
	struct sockaddr address;
	socklen_t length;
	int fd;
	struct fd_list *new_last;
	struct connection_state *new_state;
	memset(&address, 0, sizeof address);
	length = 0;
	fd = accept(server_socket, &address, &length);
	if(-1 == fd) return 1;
	new_state = malloc(sizeof(struct connection_state));
	new_state->fd = fd;
	/*return*/ append(connection_list, new_state);
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

int match_by_sockfd(struct connection_state *data, int *target, struct linked_list *node){
	if(node){}
	return data->fd == *target;
}

int handle_chunk(int sockfd, struct fd_list *connections, struct linked_list *connection_list){
	char buf[256+1];
	size_t len;
	struct linked_list *match_node;
	/*if(*/first_matching(connection_list, (visitor_t)(&match_by_sockfd), (struct linked_list*)(&sockfd), &match_node)/*) return 1*/;/**/
	while(connections)
		if(sockfd == connections->fd)
			break;
	if(!connections) return 1;
	/*then do stuff with (struct connection_state*)(match_node->data)*/
	buf[256] = 0;
	len = read(sockfd, buf, 256);
	buf[len] = 0;
	printf("read <<<%s>>>\n", buf);
	return 0;
}

int manage_resources_forever(int listening_socket){
	struct fd_list *connections;
	struct linked_list *connection_list;
	struct timeval timeout;
	int ready_fd;
	connections = malloc(sizeof(struct fd_list));
	connection_list = malloc(sizeof(struct linked_list));
	connection_list->data = 0;
	connection_list->next = 0;
	connections->fd = listening_socket;
	connections->next = 0;
	while(1){
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if(!await_a_resource(listening_socket, connections, &timeout, &ready_fd)){
			if(ready_fd == listening_socket){
				accept_new_connection(listening_socket, connections, connection_list);
			}
			else{
				handle_chunk(ready_fd, connections->next, connection_list->next);
			}
		}
	}
	free_fd_list_closing(connections->next);
	free_connection_list(connection_list->next);
	connection_list->next = 0;
	free(connection_list);
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
