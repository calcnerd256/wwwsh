/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <unistd.h>
#include <string.h>
#include "./server.h"
#include "./request.h"
#include "./static.h"


int match_resource_url(struct staticGetResource *resource, struct extent *url, struct linked_list *node){
	(void)node;
	if(!resource) return 0;
	if(!url) return 0;
	if(!(resource->url)) return 0;
	if(resource->url->len != url->len) return 0;
	return !strncmp(resource->url->bytes, url->bytes, url->len + 1);
}

int staticGetResource_urlMatchesp(struct httpResource *resource, struct extent *url){
	if(!resource) return 0;
	if(!(resource->context)) return 0;
	if(!url) return 0;
	return match_resource_url(resource->context, url, 0);
}

int staticGetResource_respond(struct httpResource *resource, struct conn_bundle *connection){
	struct extent reason;
	struct staticGetResource *response;
	if(!resource) return 1;
	if(!connection) return 1;
	response = (struct staticGetResource*)(resource->context);
	if(!response) return 1;
	if(!(connection->input.method))
		return connection_bundle_respond_bad_method(connection);
	if(3 != connection->input.method->len)
		return connection_bundle_respond_bad_method(connection);
	if(!(connection->input.method->bytes))
		return connection_bundle_respond_bad_method(connection);
	if(strncmp(connection->input.method->bytes, "GET", connection->input.method->len + 1))
		return connection_bundle_respond_bad_method(connection);
	point_extent_at_nice_string(&reason, "OK");
	return connection_bundle_send_response(connection, 200, &reason, response->headers, response->body);
}


struct contiguousHtmlResource{
	struct linked_list link_node;
	struct httpResource resource;
	struct staticGetResource staticResource;
	struct extent url;
	struct extent body;
	struct linked_list headerTop;
	struct linked_list keyNode;
	struct linked_list valNode;
	struct extent key;
	struct extent val;
};

int contiguousHtmlResource_init(struct contiguousHtmlResource *res, char *url, char *body){
	if(!res) return 2;
	if(!url) return 3;
	if(!body) return 3;
	if(point_extent_at_nice_string(&(res->url), url)) return 4;
	if(point_extent_at_nice_string(&(res->body), body)) return 4;
	res->staticResource.url = &(res->url);
	res->staticResource.body = &(res->body);
	res->staticResource.headers = push_header_nice_strings(&(res->headerTop), &(res->keyNode), &(res->valNode), &(res->key), "Content-Type", &(res->val), "text/html", 0);
	return !(res->staticResource.headers);
}

int main(int argument_count, char* *arguments_vector){
	struct httpServer server;
	struct mempool serverAllocations;
	struct linked_list *match_node;
	int ready_fd = -1;
	int status = 0;

	struct contiguousHtmlResource rootResourceStorage;

	if(2 != argument_count) return 1;
	if(httpServer_init(&server, &serverAllocations)) return 2;

	status = contiguousHtmlResource_init(
		&rootResourceStorage,
		"/",
		"<html>\r\n <head>\r\n  <title>Hello World!</title>\r\n </head>\r\n <body>\r\n  <h1>Hello, World!</h1>\r\n  <p>\r\n   This webserver is written in C.\r\n   I'm pretty proud of it!\r\n  </p>\r\n  <p>Coming soon: a resource for the following form to POST to</p>\r\n  <form method=\"POST\" action=\"formtest/\">\r\n   <input type=\"submit\" />\r\n  </form>\r\n </body>\r\n</html>\r\n\r\n"
	);
	if(status) return 3;
	status = httpServer_pushResource(&server, &(rootResourceStorage.link_node), &(rootResourceStorage.resource), &staticGetResource_urlMatchesp, &staticGetResource_respond, &(rootResourceStorage.staticResource));
	if(status) return 4;

	if(httpServer_listen(&server, arguments_vector[1], 32)){
		server.listeningSocket_fileDescriptor = -1;
		httpServer_close(&server);
		return 5;
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
		return 6;
	}
	return 0;
}
