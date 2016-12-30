/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "./chunkStream.h"

struct httpServer{
	struct mempool *memoryPool;
	int listeningSocket_fileDescriptor;
	struct linked_list *connections;
	struct linked_list *staticResources;
};

struct staticGetResource{
	struct extent *url;
	struct extent *body;
	struct linked_list *headers;
};

struct conn_bundle{
	struct mempool *pool;
	struct chunkStream *chunk_stream;
	struct chunkStream *body;
	struct extent *method;
	struct extent *request_url;
	struct httpServer *server;
	struct dequoid *request_headers;
	unsigned long int request_length;
	int fd;
	int http_major_version;
	int http_minor_version;
	char done_reading;
	char done_writing;
	char done_reading_headers;
};

int init_connection(struct conn_bundle *ptr, struct httpServer *server, int fd){
	char *p = 0;
	ptr->pool = palloc(server->memoryPool, sizeof(struct mempool));
	init_pool(ptr->pool);
	ptr->fd = fd;
	fd = 0;
	ptr->done_reading = 0;
	ptr->request_length = 0;
	ptr->method = 0;
	ptr->request_url = 0;
	ptr->http_major_version = -1;
	ptr->http_minor_version = -1;
	ptr->done_writing = 0;
	ptr->server = server;
	server = 0;
	p = palloc(ptr->pool, 2 * sizeof(struct chunkStream) + sizeof(struct dequoid));
	ptr->chunk_stream = (struct chunkStream*)p;
	p += sizeof(struct chunkStream);
	ptr->body = (struct chunkStream*)p;
	p += sizeof(struct chunkStream);
	ptr->request_headers = (struct dequoid*)p;
	p = 0;
	chunkStream_init(ptr->chunk_stream, ptr->pool);
	chunkStream_init(ptr->body, ptr->pool);
	ptr->done_reading_headers = 0;
	dequoid_init(ptr->request_headers);
	ptr = 0;
	return 0;
}

#include "./get_socket.c"
#include "./visit_connection_bundle_process_step.c"

int httpServer_init(struct httpServer *server){
	char *ptr;
	struct staticGetResource *root;
	struct linked_list *node;
	server->listeningSocket_fileDescriptor = -1;
	server->memoryPool = malloc(sizeof(struct mempool));
	init_pool(server->memoryPool);
	server->connections = 0;
	ptr = 0;
	server->staticResources = 0;

	ptr = palloc(server->memoryPool, 4 * sizeof(struct linked_list) + sizeof(struct staticGetResource) + 4 * sizeof(struct extent));
	server->staticResources = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	root = (struct staticGetResource*)ptr;
	ptr += sizeof(struct staticGetResource);
	root->url = (struct extent*)ptr;
	ptr += sizeof(struct extent);
	root->body = (struct extent*)ptr;
	ptr += sizeof(struct extent);
	root->headers = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	node = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	node->next = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	node->data = (struct extent*)ptr;
	ptr += sizeof(struct extent);
	node->next->data = (struct extent*)ptr;
	ptr = 0;

	server->staticResources->data = root;
	server->staticResources->next = 0;
	server = 0;

	point_extent_at_nice_string(root->url, "/");
	point_extent_at_nice_string(root->body, "<html>\r\n <head>\r\n  <title>Hello World!</title>\r\n </head>\r\n <body>\r\n  <h1>Hello, World!</h1>\r\n  <p>\r\n   This webserver is written in C.\r\n   I'm pretty proud of it!\r\n  </p>\r\n </body>\r\n</html>\r\n\r\n");
	root->headers->data = node;
	root->headers->next = 0;
	root = 0;
	point_extent_at_nice_string(node->data, "Content-Type");
	point_extent_at_nice_string(node->next->data, "text/html");
	node->next->next = 0;
	node = 0;
	return 0;
}

int httpServer_listen(struct httpServer *server, char* port_number, int backlog){
	int sockfd = -1;
	if(get_socket(port_number, &sockfd)){
		server = 0;
		port_number = 0;
		backlog = 0;
		sockfd = 0;
		return 1;
	}
	port_number = 0;
	server->listeningSocket_fileDescriptor = sockfd;
	server = 0;
	if(listen(sockfd, backlog)){
		backlog = 0;
		sockfd = 0;
		return 2;
	}
	sockfd = 0;
	backlog = 0;
	return 0;
}

int httpServer_close(struct httpServer *server){
	if(server->memoryPool){
		free_pool(server->memoryPool);
		free(server->memoryPool);
		server->memoryPool = 0;
	}
	server->staticResources = 0;
	if(-1 == server->listeningSocket_fileDescriptor){
		server = 0;
		return 0;
	}
	if(shutdown(server->listeningSocket_fileDescriptor, SHUT_RDWR)){
		server = 0;
		return 1;
	}
	if(close(server->listeningSocket_fileDescriptor)){
		server = 0;
		return 2;
	}
	server->listeningSocket_fileDescriptor = -1;
	server = 0;
	return 0;
}

int visit_connection_bundle_select_read(struct conn_bundle *conn, struct linked_list *context, struct linked_list *node){
	int *done;
	int *fd;
	struct timeval timeout;
	fd_set to_read;
	int status;
	(void)node;
	if(!context) return 1;
	if(!context->next) return 1;
	done = (int *)(context->data);
	if(!done) return 3;
	if(*done) return 0;
	fd = (int *)(context->next->data);
	if(!fd) return 3;
	if(!conn) return 2;
	if(conn->done_reading) return 0;
	if(-1 == conn->fd) return 2;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	FD_ZERO(&to_read);
	FD_SET(conn->fd, &to_read);
	status = select(conn->fd + 1, &to_read, 0, 0, &timeout);
	if(-1 == status) return 4;
	if(!status) return 0;
	if(FD_ISSET(conn->fd, &to_read)){
		*fd = conn->fd;
		*done = 1;
	}
	return 0;
}

int httpServer_selectRead(struct httpServer *server){
	int ready_fd;
	struct linked_list context;
	struct linked_list fd_cell;
	struct conn_bundle fake_for_server;
	struct linked_list extra_head;
	int done;
	done = 0;
	ready_fd = -1;
	fd_cell.next = 0;
	fd_cell.data = &ready_fd;
	context.next = &fd_cell;
	context.data = &done;
	extra_head.next = server->connections;
	extra_head.data = &fake_for_server;
	fake_for_server.fd = server->listeningSocket_fileDescriptor;
	fake_for_server.done_reading = 0;
	if(traverse_linked_list(&extra_head, (visitor_t)(&visit_connection_bundle_select_read), &context)) return -1;
	return ready_fd;
}

int httpServer_stepConnections(struct httpServer *server){
	int any = 0;
	if(traverse_linked_list(server->connections, (visitor_t)(&visit_connection_bundle_process_step), &any)) return 0;
	return any;
}

int httpServer_acceptNewConnection(struct httpServer *server){
	struct sockaddr address;
	socklen_t length;
	int fd;
	char *ptr;
	struct linked_list *new_head;

	memset(&address, 0, sizeof(struct sockaddr));
	length = 0;
	fd = accept(server->listeningSocket_fileDescriptor, &address, &length);
	if(-1 == fd) return 1;

	ptr = palloc(server->memoryPool, sizeof(struct conn_bundle) + sizeof(struct linked_list));
	new_head = (struct linked_list*)ptr;
	new_head->next = server->connections;
	server->connections = new_head;
	new_head->data = ptr + sizeof(struct linked_list);
	init_connection((struct conn_bundle*)(new_head->data), server, fd);
	return 0;
}

int match_by_sockfd(struct conn_bundle *data, int *target, struct linked_list *node){
	(void)node;
	return data->fd == *target;
}

int httpRequestHandler_readChunk(struct conn_bundle *conn){
	char buf[CHUNK_SIZE + 1];
	size_t len;

	if(!conn) return 2;

	buf[CHUNK_SIZE] = 0;
	len = read(conn->fd, buf, CHUNK_SIZE);
	buf[len] = 0;
	chunkStream_append(conn->chunk_stream, buf, len);

	if(len) return 0;
	conn->done_reading = 1;
	if(conn->done_writing)
		return connection_bundle_free(conn);
	return 0;
}

int main(int argument_count, char* *arguments_vector){
	struct httpServer server;
	int ready_fd;
	struct linked_list *match_node;
	if(2 != argument_count) return 1;
	if(httpServer_init(&server)) return 1;
	if(httpServer_listen(&server, arguments_vector[1], 32)){
		server.listeningSocket_fileDescriptor = -1;
		httpServer_close(&server);
		return 2;
	}

	while(1){
		ready_fd = httpServer_selectRead(&server);
		if(-1 != ready_fd){
			if(ready_fd == server.listeningSocket_fileDescriptor)
				httpServer_acceptNewConnection(&server);
			else{
				match_node = 0;
				if(!first_matching(server.connections, (visitor_t)(&match_by_sockfd), (struct linked_list *)(&ready_fd), &match_node))
					if(match_node)
						httpRequestHandler_readChunk(match_node->data);
			}
		}
		if(!httpServer_stepConnections(&server))
			if(ready_fd == -1)
				usleep(10);
	}

	if(httpServer_close(&server)){
		server.listeningSocket_fileDescriptor = -1;
		return 4;
	}
	return 0;
}
