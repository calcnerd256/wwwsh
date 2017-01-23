/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include "./server.h"
#include "./request.h"
#include "./static.h"


int spawnForm_respond_POST(struct httpResource *res, struct incomingHttpRequest *req, struct linked_list *formData){
	char pid[256];
	struct extent fieldName;
	struct childProcessResource *child;
	struct httpServer *server;
	struct staticFormResource *fr;
	struct linked_list *match_node;
	int plen;
	if(!res) return 1;
	if(!req) return 1;
	if(!formData) return 1;
	fr = res->context;
	if(!fr) return 1;
	server = ((struct linked_list*)(fr->context))->data;
	if(!server) return 1;

	match_node = 0;
	if(point_extent_at_nice_string(&fieldName, "cmd")) return 2;
	if(first_matching(formData, (visitor_t)&match_header_key, (void*)&fieldName, &match_node)) return 2;
	if(!match_node) return 2;
	if(!(match_node->data)) return 2;
	match_node = match_node->data;
	if(!(match_node->next)) return 2;
	match_node = match_node->next;
	if(!(match_node->data)) return 2;

	child = malloc(sizeof(struct childProcessResource));
	if(childProcessResource_init_spawn(child, match_node->data))
		return 3;
	if(httpServer_pushChildProcess(server, child))
		return 4;
	memset(pid, 0, 256);
	plen = snprintf(pid, 256, "%d", child->pid);
	pid[255] = 0;
	if(-1 == plen) return 8;
	if(255 < plen) return 9;
	pid[plen] = 0;

	if(incomingHttpRequest_beginChunkedHtmlOk(req, 0))
		return 5;
	if(incomingHttpRequest_writelnChunk_niceString(req, "<html>"))
		return 5;
	if(incomingHttpRequest_writelnChunk_niceString(req, " <head>"))
		return 5;
	if(incomingHttpRequest_writelnChunk_niceString(req, "  <title>spawned a process</title>"))
		return 5;
	if(incomingHttpRequest_writelnChunk_niceString(req, " </head>"))
		return 5;
	if(incomingHttpRequest_writelnChunk_niceString(req, " <body>"))
		return 5;
	if(incomingHttpRequest_writeChunk_niceString(req, "  <a href=\""))
		return 5;
	if(incomingHttpRequest_writeChunk_htmlSafeExtent(req, &(child->url)))
		return 6;
	if(incomingHttpRequest_writelnChunk_niceString(req, "/\">spawned</a>"))
		return 7;
	if(incomingHttpRequest_writelnChunk_niceString(req, " </body>"))
		return 7;
	if(incomingHttpRequest_writelnChunk_niceString(req, "</html>"))
		return 7;
	return incomingHttpRequest_sendLastChunk(req, 0);
}


int index_respond(struct httpResource *resource, struct incomingHttpRequest *request){
	struct httpServer *server;
	struct linked_list *cursor;
	struct extent *url;
	if(!resource) return 1;
	if(!request) return 1;
	server = resource->context;
	if(!server) return 1;
	cursor = server->resources;

	if(!(request->input.method))
		return incomingHttpRequest_respond_badMethod(request);
	if(3 != request->input.method->len)
		return incomingHttpRequest_respond_badMethod(request);
	if(strncmp(request->input.method->bytes, "GET", 4))
		return incomingHttpRequest_respond_badMethod(request);

	if(incomingHttpRequest_beginChunkedHtmlBody(request, 0, "index", 5)) return 2;
	if(!cursor)
		incomingHttpRequest_writelnChunk_niceString(request, "  Nothing here yet.");
	incomingHttpRequest_writelnChunk_niceString(request, "  <ul>");
	while(cursor){
		url = &(((struct httpResource*)(cursor->data))->url);
		if(url->len && url->bytes){
			incomingHttpRequest_writelnChunk_niceString(request, "   <li>");
			incomingHttpRequest_writeChunk_niceString(request, "    <a href=\"");
			incomingHttpRequest_writeChunk_htmlSafeExtent(request, url);
			incomingHttpRequest_writelnChunk_niceString(request, "\">");
			incomingHttpRequest_writeChunk_niceString(request, "     ");
			incomingHttpRequest_writeChunk_htmlSafeExtent(request, url);
			incomingHttpRequest_writelnChunk_niceString(request, "");
			incomingHttpRequest_writelnChunk_niceString(request, "    </a>");
			incomingHttpRequest_writelnChunk_niceString(request, "   </li>");
		}
		cursor = cursor->next;
	}
	incomingHttpRequest_writelnChunk_niceString(request, "  </ul>");
	return incomingHttpRequest_endChunkedHtmlBody(request, 0);
}


struct event{
	int (*precondition)(struct event*, void*);
	struct linked_list* (*step)(struct event*, void*);
	int nanoseconds_checkAgain;
	void *context;
};


int event_precondition_serverListen_accept(struct event *evt, void *env){
	struct httpServer *server;
	(void)env;
	env = 0;
	if(!evt) return 0;
	server = evt->context;
	evt = 0;
	return fd_canReadp(server->listeningSocket_fileDescriptor);
}
struct linked_list* event_step_serverListen_accept(struct event *evt, void *env){
	struct linked_list *result;
	struct event *again;
	struct httpServer *server;
	(void)env;
	env = 0;
	if(!evt) return 0;
	server = evt->context;

	httpServer_acceptNewConnection(server);

	result = malloc(sizeof(struct linked_list));
	result->next = 0;
	result->data = malloc(sizeof(struct event));
	again = result->data;
	again->context = server;
	again->nanoseconds_checkAgain = 1000 * 100;
	again->precondition = &event_precondition_serverListen_accept;
	again->step = &event_step_serverListen_accept;
	again = 0;
	return result;
}

int event_precondition_stepConnections(struct event *evt, void *env){
	struct httpServer *server;
	(void)env;
	env = 0;
	if(!evt) return 0;
	server = evt->context;
	evt = 0;
	if(!server) return 0;
	return httpServer_stepConnections(server);
}
struct linked_list* event_step_stepConnections(struct event *evt, void *env){
	struct linked_list *result_node;
	struct event *result_data;
	(void)env;
	env = 0;
	if(!evt) return 0;
	result_data = malloc(sizeof(struct event));
	result_data->precondition = &event_precondition_stepConnections;
	result_data->step = &event_step_stepConnections;
	result_data->nanoseconds_checkAgain = 1000 * 100;
	result_data->context = evt->context;
	result_node = malloc(sizeof(struct linked_list));
	result_node->next = 0;
	result_node->data = result_data;
	return result_node;
}

int main(int argument_count, char* *arguments_vector){
	struct httpServer server;
	int status = 0;

	struct contiguousHtmlResource rootResourceStorage;

	struct staticFormResource formResource;
	struct mempool formAllocations;
	struct linked_list formContext;
	struct linked_list formPoolCell;
	struct linked_list fieldHead;
	struct linked_list fieldNameNode;
	struct linked_list fieldTagNode;
	struct extent fieldName;
	struct extent fieldTag;
	struct form formForm;
	struct httpResource indexResource;
	struct linked_list indexResource_linkNode;

	struct event *serverListen;
	struct linked_list *garbage;
	struct event *currentEvent;
	int minimumSleep = -1;
	struct linked_list *new_head;
	struct event *candidate;
	struct linked_list *temp_node;
	struct dequoid events;

	if(2 > argument_count) return 1;
	if(3 < argument_count) return 1;
	if(2 < argument_count)
		if(!atoi(arguments_vector[2]))
			return 12;

	if(point_extent_at_nice_string(&fieldName, "cmd")) return 11;
	if(point_extent_at_nice_string(&fieldTag, "textarea")) return 11;
	if(init_pool(&formAllocations)) return 8;
	if(httpServer_init(&server)) return 2;

	status = contiguousHtmlResource_init(
		&rootResourceStorage,
		"/",
		"<html>\r\n"
		" <head>\r\n"
		"  <title>Hello World!</title>\r\n"
		" </head>\r\n"
		" <body>\r\n"
		"  <h1>wwwsh</h1>\r\n"
		"  <p>\r\n"
		"   a big, gaping security vulnerability that lets anyone\r\n"
		"   <a href=\"./spawn/\">spawn a process</a>\r\n"
		"  </p>\r\n"
		"  <p>\r\n"
		"   see <a href=\"/index/\">all</a> resources"
		"  </p>\r\n"
		" </body>\r\n"
		"</html>\r\n"
	);
	if(status) return 3;
	status = httpServer_pushResource(
		&server,
		&(rootResourceStorage.link_node),
		&(rootResourceStorage.resource),
		0,
		&staticGetResource_canRespondp,
		&staticGetResource_respond,
		&(rootResourceStorage.staticResource)
	);
	if(status) return 4;
	/*
		You MUST NOT set rootResourceStorage.link_node.data to 0,
		or else the server will try to free stacked memory.
		If you wish to remove this cell from the linked list,
		then you must remove it by pointing its predecessor at its successor.
	*/

	status = httpServer_pushResource(
		&server,
		&indexResource_linkNode,
		&indexResource,
		0,
		&staticGetResource_canRespondp,
		&index_respond,
		&server
	);
	if(status) return 4;
	/*
		You MUST NOT set indexResource_linkNode.data to 0,
		or else the server will try to free stacked memory.
		If you wish to remove this cell from the linked list,
		then you must remove it by pointing its predecessor at its successor.
	*/
	if(point_extent_at_nice_string(&(indexResource.url), "/index/")) return 4;

	formContext.data = &server;
	formContext.next = &formPoolCell;
	formPoolCell.data = &formAllocations;
	formPoolCell.next = 0;

	fieldHead.data = &fieldNameNode;
	fieldHead.next = 0;
	fieldNameNode.data = &fieldName;
	fieldNameNode.next = &fieldTagNode;
	fieldTagNode.data = &fieldTag;
	fieldTagNode.next = 0;

	status = staticFormResource_init(
		&formResource,
		&server,
		&formForm,
		"/spawn/",
		"spawn",
		&fieldHead,
		&spawnForm_respond_POST,
		&formContext
	);
	if(status)
		return 7;
	/*
		You MUST NOT set formResource.node.data to 0,
		or else the server will try to free stacked memory.
		If you wish to remove this cell from the linked list,
		then you must remove it by pointing its predecessor at its successor.
	 */


	if(httpServer_listen(&server, arguments_vector[1], 32)){
		server.listeningSocket_fileDescriptor = -1;
		httpServer_close(&server);
		return 5;
	}

	if(2 < argument_count)
		if(kill(atoi(arguments_vector[2]), SIGCONT)){
			httpServer_close(&server);
			return 13;
		}

	serverListen = malloc(sizeof(struct event));
	serverListen->context = &server;
	serverListen->nanoseconds_checkAgain = 1000 * 100;
	serverListen->precondition = &event_precondition_serverListen_accept;
	serverListen->step = &event_step_serverListen_accept;
	dequoid_init(&events);
	dequoid_append(&events, serverListen, malloc(sizeof(struct linked_list)));
	serverListen = malloc(sizeof(struct event));
	serverListen->context = &server;
	serverListen->nanoseconds_checkAgain = 1000 * 100;
	serverListen->precondition = &event_precondition_stepConnections;
	serverListen->step = &event_step_stepConnections;
	dequoid_append(&events, serverListen, malloc(sizeof(struct linked_list)));
	new_head = 0;

	while(1){
		minimumSleep = -1;
		currentEvent = 0;
		new_head = events.head;
		garbage = 0;
		while(new_head){
			temp_node = new_head->next;
			if(temp_node)
				if(!(temp_node->data)){
					new_head->next = temp_node->next;
					temp_node->next = 0;
					free(temp_node);
				}
			candidate = new_head->data;
			if(candidate){
				if(!(candidate->precondition))
					garbage = new_head;
				else
					if((*(candidate->precondition))(candidate, 0))
						garbage = new_head;
				if(-1 == minimumSleep)
					minimumSleep = candidate->nanoseconds_checkAgain;
				if(minimumSleep > candidate->nanoseconds_checkAgain)
					minimumSleep = candidate->nanoseconds_checkAgain;
			}
			new_head = new_head->next;
		}
		if(garbage)
			if(garbage->data)
				currentEvent = garbage->data;
		if(currentEvent){
			garbage->data = 0;
			garbage = 0;
			if(currentEvent->step)
				garbage = (*(currentEvent->step))(currentEvent, 0);
			free(currentEvent);
			while(garbage){
				temp_node = garbage->next;
				if(garbage->data)
					dequoid_append(&events, garbage->data, malloc(sizeof(struct linked_list)));
				garbage->next = 0;
				garbage->data = 0;
				free(garbage);
				garbage = temp_node;
			}
		}
		else
			if(minimumSleep > 1000)
				usleep(minimumSleep / 1000);
	}

	free(serverListen);
	serverListen = 0;

	if(httpServer_close(&server)){
		server.listeningSocket_fileDescriptor = -1;
		return 6;
	}
	return 0;
}
