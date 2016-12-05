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

struct connection_state{
	struct linked_list *request;
	struct linked_list *lines;
	int fd;
	char done_reading;
	struct linked_list *last_parsed_line;
	char *method;
	char *request_uri;
	int http_maj_ver;
	int http_min_ver;
};

int init_connection_state(struct connection_state *ptr){
	ptr->fd = 0;
	ptr->request = malloc(sizeof(struct linked_list));
	ptr->request->data = 0;
	ptr->request->next = 0;
	ptr->lines = malloc(sizeof(struct linked_list));
	ptr->lines->data = 0;
	ptr->lines->next = 0;
	ptr->done_reading = 0;
	ptr->last_parsed_line = 0;
	ptr->method = 0;
	ptr->request_uri = 0;
	ptr->http_maj_ver = -1;
	ptr->http_min_ver = -1;
	return 0;
}

int request_method_is_GET_p(char *line){
	if('G' != *line) return 0;
	if('E' != line[1]) return 0;
	if('T' != line[2]) return 0;
	if(' ' != line[3]) return 0;
	return 1;
}
int path_is_root_p(char *suffix){
	if('/' != *suffix) return 0;
	if(' ' != suffix[1]) return 0;
	return 1;
}
int http_maj_ver_is_one_p(char *suffix){
	if('H' != *suffix) return 0;
	if('T' != suffix[1]) return 0;
	if('T' != suffix[2]) return 0;
	if('P' != suffix[3]) return 0;
	if('/' != suffix[4]) return 0;
	if('1' != suffix[5]) return 0;
	if('.' != suffix[6]) return 0;
	return 1;
}
int http_min_ver_is_one_p(char *suffix){
	if('1' != *suffix) return 0;
	if('\r' != suffix[1]) return 0;
	if('\n' != suffix[2]) return 0;
	if(suffix[3]) return 0;
	return 1;
}

int parse_request_line(struct connection_state *conn, char *line){
	/*
		Request-Line is RFC 2616 section 5.1
		https://tools.ietf.org/html/rfc2616#section-5.1
	*/
	switch(*line){
		case 'G':
			if(request_method_is_GET_p(line)){
				conn->method = "GET";
				if(path_is_root_p(line + 4)){
					conn->request_uri = "/";
					if(http_maj_ver_is_one_p(line + 6)){
						conn->http_maj_ver = 1;
						if(http_min_ver_is_one_p(line + 13)){
							conn->http_min_ver = 1;
							printf("GET request on root, version 1.1\n");
							return 0;
						}
						return 1;
					}
					return 1;
				}
				return 1;
			}
			return 1;
			break;
	}
	return 1;
}

int parse_from_lines(struct connection_state *conn){
	struct linked_list *next_line_to_parse = 0;
	char *current_line = 0;
	int status;
	if(!conn->last_parsed_line) next_line_to_parse = conn->lines->next;
	else next_line_to_parse = conn->last_parsed_line->next;
	/* so now we start parsing from next_line_to_parse */
	if(!next_line_to_parse) return 0;
	if(!conn->method){
		conn->last_parsed_line = next_line_to_parse;
		current_line = flatten_ransom(next_line_to_parse->data);
		status = parse_request_line(conn, current_line);
		free(current_line);
		if(status) return status;
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

int visit_free_string(char *buf, void *context, struct linked_list *node){
	free(buf);
	(void)context;
	(void)node;
	return 0;
}
int clean_connection(struct connection_state *state, void *context, struct linked_list *node){
	close(state->fd);
	state->fd = 0;
	clean_and_free_list(state->request->next, (visitor_t)(&visit_free_string), context);
	state->request->next = 0;
	free(state->request);
	state->request = 0;
	clean_and_free_list(state->lines->next, (visitor_t)(&visit_clean_ransom), context);
	state->lines->next = 0;
	free(state->lines);
	state->lines = 0;
	state->done_reading = 1;
	(void)node;
	return 0;
}
int free_connection_list(struct linked_list *head){
	int status;
	struct linked_list *ptr;
	status = traverse_linked_list(head, (visitor_t)(&clean_connection), 0);
	if(status) return status;
	while(head){
		head->data = 0;
		ptr = head->next;
		head->next = 0;
		head = ptr;
	}
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

int accept_new_connection(int server_socket, struct linked_list *connection_list, struct mempool *allocations){
	struct sockaddr address;
	socklen_t length;
	int fd;
	struct connection_state *new_state;
	char *ptr;
	memset(&address, 0, sizeof address);
	length = 0;
	fd = accept(server_socket, &address, &length);
	if(-1 == fd) return 1;
	connection_list = last_node(connection_list);
	if(!connection_list) return 1;

	ptr = palloc(allocations, sizeof(struct connection_state) + sizeof(struct linked_list));
	new_state = (struct connection_state*)(ptr + sizeof(struct linked_list));
	connection_list->next = (struct linked_list*)ptr;

	init_connection_state(new_state);
	new_state->fd = fd;
	connection_list = connection_list->next;
	connection_list->data = new_state;
	connection_list->next = 0;
	return 0;
}

int match_by_sockfd(struct connection_state *data, int *target, struct linked_list *node){
	if(node){}
	return data->fd == *target;
}

int visit_print_string(char *buf, void *context, struct linked_list *node){
	printf("%s", buf);
	return (int)(0 * (long)context * (long)node);
}

int index_of_first_linebreak(char *buf, char carry){
	/* returns the character _ending_ the CRLF */
	int result = 0;
	if(!buf) return -1;
	if(carry)
		if('\n' == *buf)
			return 0;
	while(*buf){
		result++;
		if('\r' == *buf)
			if('\n' == buf[1]) return result;
		buf++;
	}
	return -1;
}

int parse_lines_from_chunk(struct linked_list *lines, char *chunk){
	struct linked_list *last_line_node;
	struct linked_list *last_line;
	int next_linebreak;
	char carry;
	struct linked_list *fray_node;
	struct extent *fray;
	last_line_node = last_node(lines);
	if(!last_line_node) return 1;
	if(!last_line_node->data){
		/* the first line */
		append(last_line_node, malloc(sizeof(struct linked_list)));
		last_line_node = last_line_node->next;
		init_ransom(last_line_node->data);
	}
	last_line = last_line_node->data;
	carry = 0;
	fray_node = last_node(last_line);
	if(fray_node)
		if(fray_node->data){
			fray = (struct extent*)(fray_node->data);
			if(fray->len)
				if('\r' == fray->bytes[fray->len - 1])
					carry = 1;
		}
	while(1){
		next_linebreak = index_of_first_linebreak(chunk, carry);
		if(-1 == next_linebreak) break;
		append_to_ransom(last_line, chunk, next_linebreak+1);
		last_line = malloc(sizeof(struct linked_list));
		append(last_line_node, last_line);
		last_line_node = last_line_node->next;
		init_ransom(last_line);
		chunk += next_linebreak + 1;
	}
	next_linebreak = strlen(chunk);
	if(!next_linebreak) return 0;
	return append_to_ransom(last_line, chunk, next_linebreak);
}

int handle_chunk(int sockfd, struct linked_list *connection_list){
	char *buf;
	size_t len;
	struct linked_list *match_node;
	struct connection_state *conn;
	if(first_matching(connection_list, (visitor_t)(&match_by_sockfd), (struct linked_list*)(&sockfd), &match_node))
		return 1;
	buf = malloc(CHUNK_SIZE + 1);
	buf[CHUNK_SIZE] = 0;
	len = read(sockfd, buf, CHUNK_SIZE);
	buf[len] = 0;
	conn = (struct connection_state*)(match_node->data);
	if(!conn){return 2;}
	if(!len){
		conn->done_reading = 1;
		printf("request socket with file descriptor %d closed; body follows:\n", conn->fd);
		traverse_linked_list(conn->request->next, (visitor_t)(&visit_print_string), 0);
		printf("\n\nDone.\n");
		printf("by line:\n");
		traverse_linked_list(conn->lines->next, (visitor_t)&visit_print_ransom, (void*)1);
		printf("\n\nDone.\n");
		memset(buf, 0, CHUNK_SIZE+1);
		free(buf);
		return 0;
	}
	append(conn->request, (void*)buf);
	printf("read %d bytes\n", (int)len);
	if(parse_lines_from_chunk(conn->lines, buf)) return 3;
	if(parse_from_lines(conn)){
		printf("failed to parse HTTP request");
		return 4;
	}
	return 0;
}

int manage_resources_forever(int listening_socket){
	struct linked_list connection_list;
	struct timeval timeout;
	int ready_fd;
	struct mempool pool;
	connection_list.data = 0;
	connection_list.next = 0;
	init_pool(&pool);
	while(1){
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if(!await_a_resource(listening_socket, &connection_list, &timeout, &ready_fd)){
			if(ready_fd == listening_socket){
				accept_new_connection(listening_socket, &connection_list, &pool);
			}
			else{
				handle_chunk(ready_fd, connection_list.next);
			}
		}
	}
	free_connection_list(connection_list.next);
	connection_list.next = 0;
	free_pool(&pool);
	return 0;
}

/*
int test_pools(){
	struct mempool pool;
	char *str;
	if(init_pool(&pool)) return 1;
	str = palloc(&pool, 3);
	str[2] = 0;
	str[0] = 'O';
	str[1] = 'K';
	printf("%p={allocs: %p=<%p[:%d]=%p=%p=\"%s\", %p>}\n", &pool, pool.allocs, pool.allocs->data, ((struct extent*)(pool.allocs->data))->len, ((struct extent*)(pool.allocs->data))->bytes, str, str, pool.allocs->next);
	free_pool(&pool);
	return 0;
}
*/



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
