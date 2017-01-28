/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "./event.h"


int events_minimumSleep(struct linked_list *events){
	int result = -1;
	struct event *current;
	while(events){
		current = events->data;
		if(current){
			if(-1 == result)
				result = current->nanoseconds_checkAgain;
			if(-1 != current->nanoseconds_checkAgain)
				if(current->nanoseconds_checkAgain < result)
					result = current->nanoseconds_checkAgain;
		}
		events = events->next;
	}
	return result;
}

int events_whichPreconditionMet(struct linked_list *events, struct linked_list* *out){
	struct event *candidate;
	while(events){
		candidate = events->data;
		if(candidate){
			if(!(candidate->precondition)){
				*out = events;
				return 0;
			}
			if((*(candidate->precondition))(candidate, 0)){
				*out = events;
				return 0;
			}
		}
		events = events->next;
	}
	return 0;
}

int events_stepOrSleep(struct dequoid *events){
	int minimumSleep;
	struct linked_list *tempNode;
	struct linked_list *node = 0;
	struct event *currentEvent = 0;
	struct timespec nanotime;
	linkedList_popEmptyFreeing(&(events->head));
	if(linkedList_removeMiddleEmptiesFreeing(events->head)) return 1;
	minimumSleep = events_minimumSleep(events->head);
	if(events_whichPreconditionMet(events->head, &node)) return 2;
	if(node)
		if(node->data)
			currentEvent = node->data;
	if(currentEvent){
		node->data = 0;
		node = 0;
		if(currentEvent->step)
			node = (*(currentEvent->step))(currentEvent, 0);
		free(currentEvent);
		while(node){
			tempNode = node->next;
			if(node->data)
				dequoid_append(events, node->data, malloc(sizeof(struct linked_list)));
			node->next = 0;
			node->data = 0;
			free(node);
			node = tempNode;
		}
		return 0;
	}
	if(minimumSleep > 1000){
		usleep(minimumSleep / 1000);
		return 0;
	}
	if(!minimumSleep) return 0;
	nanotime.tv_sec = 0;
	nanotime.tv_nsec = minimumSleep;
	nanosleep(&nanotime, 0);
	return 0;
}
