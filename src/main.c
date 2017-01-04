/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "./requestInput.h"
#include "./network.h"

struct httpServer{
	struct mempool *memoryPool;
	int listeningSocket_fileDescriptor;
	struct linked_list *connections;
	struct linked_list *resources;
};

struct staticGetResource{
	struct extent *url;
	struct extent *body;
	struct linked_list *headers;
};

struct conn_bundle{
	struct requestInput input;
	struct mempool allocations;
	struct httpServer *server;
	struct linked_list *node;
	int fd;
	char done_writing;
};

struct httpResource{
	int (*urlMatchesp)(struct httpResource*, struct extent*);
	int (*respond)(struct httpResource*, struct conn_bundle*);
	void *context;
};


int init_connection(struct conn_bundle *ptr, struct httpServer *server, int fd){
	memset(&(ptr->allocations), 0, sizeof(struct mempool));
	init_pool(&(ptr->allocations));
	ptr->fd = fd;
	fd = 0;
	ptr->done_writing = 0;
	ptr->server = server;
	server = 0;
	requestInput_init(&(ptr->input), &(ptr->allocations));
	ptr->input.fd = ptr->fd;
	ptr->node = 0;
	ptr = 0;
	return 0;
}

#include "./visit_connection_bundle_process_step.c"

int httpServer_pushResource(struct httpServer *server, struct linked_list *new_head, struct httpResource *resource, int (*urlMatchesp)(struct httpResource*, struct extent*), int (*respond)(struct httpResource*, struct conn_bundle*), void *context){
	if(!server) return 1;
	if(!new_head) return 2;
	if(!urlMatchesp) return 2;
	if(!respond) return 2;
	resource->urlMatchesp = urlMatchesp;
	resource->respond = respond;
	resource->context = context;
	new_head->next = server->resources;
	new_head->data = resource;
	server->resources = new_head;
	return 0;
}

int staticGetResource_initHtml(struct staticGetResource *resource, char* url, char* body, struct extent *urlStorage, struct extent *bodyStorage, struct linked_list *headerHeadStorage, struct linked_list *otherHeaders, struct linked_list *keyNode, struct linked_list *valueNode, struct extent *key, struct extent *value){
	point_extent_at_nice_string(urlStorage, url);
	point_extent_at_nice_string(bodyStorage, body);
	point_extent_at_nice_string(key, "Content-Type");
	point_extent_at_nice_string(value, "text/html");
	keyNode->data = key;
	valueNode->data = value;
	valueNode->next = 0;
	keyNode->next = valueNode;
	headerHeadStorage->data = keyNode;
	headerHeadStorage->next = otherHeaders;
	resource->url = urlStorage;
	resource->body = bodyStorage;
	resource->headers = headerHeadStorage;
	return 0;
}

int httpServer_pushRoot(struct httpServer *server){
	char *ptr;
	char *url;
	char *body;
	size_t resource_size = sizeof(struct linked_list) + sizeof(struct httpResource) + sizeof(struct staticGetResource);
	size_t staticResource_size = 3 * sizeof(struct linked_list) + 4 * sizeof(struct extent);
	struct linked_list *new_head;
	struct httpResource *resource_storage;
	struct staticGetResource *staticResource_storage;
	struct extent *urlStorage;
	struct extent *bodyStorage;
	struct linked_list *headerHeadStorage;
	struct linked_list *keyNode;
	struct linked_list *valueNode;
	struct extent *key;
	struct extent *value;
	ptr = palloc(server->memoryPool, resource_size + staticResource_size);
	new_head = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	resource_storage = (struct httpResource*)ptr;
	ptr += sizeof(struct httpResource);
	staticResource_storage = (struct staticGetResource*)ptr;
	ptr += sizeof(struct staticGetResource);
	url = "/";
	body = "<html>\r\n <head>\r\n  <title>Hello World!</title>\r\n </head>\r\n <body>\r\n  <h1>Hello, World!</h1>\r\n  <p>\r\n   This webserver is written in C.\r\n   I'm pretty proud of it!\r\n  </p>\r\n </body>\r\n</html>\r\n\r\n";
	urlStorage = (struct extent*)ptr;
	ptr += sizeof(struct extent);
	bodyStorage = (struct extent*)ptr;
	ptr += sizeof(struct extent);
	headerHeadStorage = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	keyNode = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	valueNode = (struct linked_list*)ptr;
	ptr += sizeof(struct linked_list);
	key = (struct extent*)ptr;
	ptr += sizeof(struct extent);
	value = (struct extent*)ptr;
	ptr += sizeof(struct extent);
	ptr = 0;
	staticGetResource_initHtml(staticResource_storage, url, body, urlStorage, bodyStorage, headerHeadStorage, 0, keyNode, valueNode, key, value);
	url = 0;
	body = 0;
	urlStorage = 0;
	bodyStorage = 0;
	headerHeadStorage = 0;
	keyNode = 0;
	valueNode = 0;
	key = 0;
	value = 0;
	return httpServer_pushResource(server, new_head, resource_storage, &staticGetResource_urlMatchesp, &staticGetResource_respond, staticResource_storage);
}

int httpServer_init(struct httpServer *server){
	server->listeningSocket_fileDescriptor = -1;
	server->memoryPool = malloc(sizeof(struct mempool));
	init_pool(server->memoryPool);
	server->connections = 0;
	server->resources = 0;
	server = 0;
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
	server->resources = 0;
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
	if(conn->input.done) return 0;
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
	fake_for_server.input.done = 0;
	if(traverse_linked_list(&extra_head, (visitor_t)(&visit_connection_bundle_select_read), &context)) return -1;
	return ready_fd;
}

int httpServer_removeEmptyConnections(struct httpServer *server){
	struct linked_list *node = 0;
	struct linked_list *middle = 0;
	if(!server) return 1;
	if(!(server->connections)) return 0;
	while(server->connections && !(server->connections->data)){
		node = server->connections->next;
		server->connections->next = 0;
		free(server->connections);
		server->connections = node;
	}
	node = server->connections;
	while(node){
		while(node->next && !(node->next->data)){
			middle = node->next;
			node->next = middle->next;
			middle->next = 0;
			free(middle);
		}
		node = node->next;
	}
	return 0;
}

int httpServer_stepConnections(struct httpServer *server){
	int any = 0;
	if(traverse_linked_list(server->connections, (visitor_t)(&visit_connection_bundle_process_step), &any)) return 0;
	httpServer_removeEmptyConnections(server);
	return any;
}

int httpServer_acceptNewConnection(struct httpServer *server){
	struct sockaddr address;
	socklen_t length;
	int fd;
	struct linked_list *new_head;

	memset(&address, 0, sizeof(struct sockaddr));
	length = 0;
	fd = accept(server->listeningSocket_fileDescriptor, &address, &length);
	if(-1 == fd) return 1;

	new_head = malloc(sizeof(struct linked_list));
	new_head->data = malloc(sizeof(struct conn_bundle));
	new_head->next = server->connections;
	init_connection((struct conn_bundle*)(new_head->data), server, fd);
	((struct conn_bundle*)(new_head->data))->node = new_head;
	server->connections = new_head;
	return 0;
}

int match_by_sockfd(struct conn_bundle *data, int *target, struct linked_list *node){
	(void)node;
	node = 0;
	if(!data) return 0;
	if(!target) return 0;
	return data->fd == *target;
}

int httpRequestHandler_readChunk(struct conn_bundle *conn){
	char buf[CHUNK_SIZE + 1];
	size_t len;

	if(!conn) return 2;

	buf[CHUNK_SIZE] = 0;
	len = read(conn->fd, buf, CHUNK_SIZE);
	buf[len] = 0;
	chunkStream_append(conn->input.chunks, buf, len);

	if(len) return 0;
	conn->input.done = 1;
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
	if(httpServer_pushRoot(&server)) return 1;
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
