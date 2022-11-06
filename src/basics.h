#ifndef SSOCKETS_LIB_BASICS_H
#define SSOCKETS_LIB_BASICS_H

#include <ssockets.h>

extern size_t SSockets_nTasks;
extern SSockets_task* SSockets_tasks;

#define SSockets_event_LISTEN  0
#define SSockets_event_TASK    1
#define SSockets_event_TIMEOUT 2
struct SSockets_event {
	int type;

	union {
		int listenfd;
		struct SSockets_ctx* ctx;
	} u;
};

extern int SSockets_epollfd;

extern SSockets_callback SSockets_hangupCallback;
extern SSockets_callback SSockets_timeoutCallback;
extern SSockets_callback SSockets_destroyCallback;

// This is critical
#define LISTEN_EVENTS (EPOLLIN | EPOLLEXCLUSIVE)
#define BASIC_EVENTS  (EPOLLET | EPOLLONESHOT)
#define TIMER_EVENTS  (EPOLLET | EPOLLIN)

// Some common includes
#include <stdio.h>
#include <stdlib.h>

#endif
