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
	int fd;
	char done_reading;
	unsigned long int request_length;
	unsigned long int cursor_position;
	size_t cursor_chunk_offset;
};

int init_connection(struct conn_bundle *ptr, struct mempool *allocations, int fd){
	ptr->pool = allocations;
	ptr->fd = fd;
	ptr->done_reading = 0;
	ptr->chunks = 0;
	ptr->request_length = 0;
	ptr->cursor_position = 0;
	ptr->last_chunk = 0;
	ptr->cursor_chunk = 0;
	ptr->cursor_chunk_offset = 0;
	ptr->lines = 0;
	ptr->last_line = 0;
	return 0;
}

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

int init_connection_state(struct connection_state *ptr, struct mempool *allocations){
	ptr->request = (struct linked_list*)palloc(allocations, 2 * sizeof(struct linked_list));
	ptr->lines = (struct linked_list*)((char*)ptr->request + sizeof(struct linked_list));
	ptr->fd = 0;
	ptr->request->data = 0;
	ptr->request->next = 0;
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
	state->request = 0;
	clean_and_free_list(state->lines->next, (visitor_t)(&visit_clean_ransom), context);
	state->lines->next = 0;
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
