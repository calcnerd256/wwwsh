/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./linkedList/headers.h"


struct event{
	int (*precondition)(struct event*, void*);
	struct linked_list* (*step)(struct event*, void*);
	int nanoseconds_checkAgain;
	void *context;
};

int events_stepOrSleep(struct dequoid *events);
