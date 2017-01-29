/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include "./server.h"
#include "./request.h"
#include "./static.h"
#include "./event.h"


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


struct event* event_allocInit(void *context, int delay, int (*predicate)(struct event*, void*), struct linked_list* (*step)(struct event*, void*)){
	struct event *result;
	result = malloc(sizeof(struct event));
	result->context = context;
	result->nanoseconds_checkAgain = delay;
	result->precondition = predicate;
	result->step = step;
	return result;
}

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
	struct httpServer *server;
	(void)env;
	env = 0;
	if(!evt) return 0;
	server = evt->context;

	httpServer_acceptNewConnection(server);

	result = malloc(sizeof(struct linked_list));
	result->next = 0;
	result->data = event_allocInit(server, 1000 * 100, &event_precondition_serverListen_accept, &event_step_serverListen_accept);
	return result;
}

int event_precondition_stepConnections(struct event *evt, void *env){
	struct linked_list* *headptr;
	int any = 0;
	(void)env;
	env = 0;
	if(!evt) return 0;
	headptr = evt->context;
	evt = 0;
	if(!headptr) return 0;

	if(traverse_linked_list(*headptr, (visitor_t)(&visit_incomingHttpRequest_processStep), &any))
		return 0;

	return any;
}
struct linked_list* event_step_stepConnections(struct event *evt, void *env){
	struct linked_list *result_node;
	struct event *result_data;
	(void)env;
	env = 0;
	if(!evt) return 0;
	result_data = event_allocInit(evt->context, 1000 * 100, &event_precondition_stepConnections, &event_step_stepConnections);
	evt = 0;
	result_node = malloc(sizeof(struct linked_list));
	result_node->next = 0;
	result_node->data = result_data;
	return result_node;
}

int event_precondition_stepChildProcesses(struct event *evt, void *env){
	struct linked_list* *headptr;
	int any = 0;
	(void)env;
	env = 0;
	if(!evt) return 0;
	headptr = evt->context;
	evt = 0;
	if(!headptr) return 0;
	if(traverse_linked_list(*headptr, (visitor_t)(&visit_childProcessResource_processStep), &any))
		return 0;
	headptr = 0;
	return any;
}
struct linked_list* event_step_stepChildProcesses(struct event *evt, void *env){
	struct linked_list *result;
	(void)env;
	env = 0;
	if(!evt) return 0;
	result = malloc(sizeof(struct linked_list));
	result->next = 0;
	result->data = event_allocInit(evt->context, 1000 * 100, &event_precondition_stepChildProcesses, &event_step_stepChildProcesses);
	evt = 0;
	return result;
}


struct linked_list* event_step_cleanupList(struct event *evt, void *env){
	struct linked_list *result_node;
	struct event *result_event;
	struct linked_list* *headptr;
	(void)env;
	env = 0;
	if(!evt) return 0;
	headptr = evt->context;
	evt = 0;
	if(!headptr) return 0;

	linkedList_popEmptyFreeing(headptr);
	linkedList_removeMiddleEmptiesFreeing(*headptr);

	result_event = event_allocInit(headptr, 1000 * 1000 * 1000, 0, &event_step_cleanupList);
	result_node = malloc(sizeof(struct linked_list));
	result_node->next = 0;
	result_node->data = result_event;
	return result_node;
}


struct linked_list* event_step_beginLoops(struct event *evt, void *env){
	struct httpServer *server;
	struct linked_list *result = 0;
	struct linked_list *temp = 0;
	(void)env;
	env = 0;
	if(!evt) return 0;
	server = evt->context;
	evt = 0;
	if(!server) return 0;
	result = malloc(sizeof(struct linked_list));
	result->next = 0;
	result->data = event_allocInit(&(server->resources), 1000 * 1000 * 1000, 0, &event_step_cleanupList);
	temp = result;
	result = malloc(sizeof(struct linked_list));
	result->next = temp;
	result->data = event_allocInit(&(server->children), 1000 * 1000 * 1000, 0, &event_step_cleanupList);
	temp = result;
	result = malloc(sizeof(struct linked_list));
	result->next = temp;
	result->data = event_allocInit(&(server->connections), 1000 * 1000 * 1000, 0, &event_step_cleanupList);
	temp = result;
	result = malloc(sizeof(struct linked_list));
	result->next = temp;
	result->data = event_allocInit(&(server->children), 1000 * 100, &event_precondition_stepChildProcesses, &event_step_stepChildProcesses);
	temp = result;
	result = malloc(sizeof(struct linked_list));
	result->next = temp;
	result->data = event_allocInit(&(server->connections), 1000 * 100, &event_precondition_stepConnections, &event_step_stepConnections);
	temp = result;
	result = malloc(sizeof(struct linked_list));
	result->next = temp;
	result->data = event_allocInit(server, 1000 * 100, &event_precondition_serverListen_accept, &event_step_serverListen_accept);
	temp = 0;
	return result;
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

	struct linked_list *garbage;
	struct linked_list *new_head;
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

	dequoid_init(&events);
	dequoid_append(
		&events,
		event_allocInit(&server, 0, 0, &event_step_beginLoops),
		malloc(sizeof(struct linked_list))
	);

	while(!events_stepOrSleep(&events));

	new_head = events.head;
	while(new_head){
		garbage = new_head->next;
		new_head->next = 0;
		if(new_head->data)
			free(new_head->data);
		new_head->data = 0;
		free(new_head);
		new_head = garbage;
	}

	if(httpServer_close(&server)){
		server.listeningSocket_fileDescriptor = -1;
		return 6;
	}
	return 0;
}
