/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

/* included in main.c */

#include "./await_a_resource.c"
#include "./accept_new_connection.c"
#include "./handle_chunk.c"

int manage_resources_forever(int listening_socket){
	struct timeval timeout;
	int ready_fd;
	struct mempool pool;
	struct linked_list *connections = 0;
	init_pool(&pool);
	while(1){
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if(!await_a_resource(listening_socket, &timeout, &ready_fd, connections)){
			if(ready_fd == listening_socket){
				accept_new_connection(listening_socket, &pool, &connections);
			}
			else{
				handle_chunk(ready_fd, connections);
			}
		}
	}
	free_pool(&pool);
	return 0;
}
