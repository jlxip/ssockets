#include <stdio.h>
#include "worker.h"
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <errno.h>

#define MAX_EVENTS 32

int setNonBlocking(int fd);

void* SSockets_worker(void*) {
	// ✨ Welcome to the worker procedure ✨

	struct epoll_event events[MAX_EVENTS];
	while(1) {
		// Wait for events
		size_t nfds = epoll_wait(SSockets_epollfd, events, MAX_EVENTS, -1);
		// Process each one
		for(size_t i=0; i<nfds; ++i) {
			struct SSockets_event* evt = events[i].data.ptr;

			size_t ev = events[i].events;
			struct SSockets_ctx* ctx = NULL;

			if(evt->type == SSockets_event_LISTEN) {
				// New connection
				struct sockaddr_in addr;
				uint32_t len = sizeof(addr);
				int conn = accept(evt->u.listenfd, (struct sockaddr*)&addr, &len);
				if(conn == -1)
					continue;
				if(setNonBlocking(conn) != 0) {
					// Something really weird happened
					close(conn);
					continue;
				}


				// Set up the task object
				ctx = malloc(sizeof(struct SSockets_ctx));
				ctx->state = 0;
				ctx->fd = conn;
				ctx->addr = addr;
				ctx->timeout = ctx->disarm = 0;
				ctx->data = NULL;

				// Create its event
				evt = malloc(sizeof(struct SSockets_event));
				evt->type = SSockets_event_TASK;
				evt->u.ctx = ctx;
				ctx->regularevt = evt;

				// And listen to it
				struct epoll_event ev;
				ev.events = BASIC_EVENTS;
				ev.data.ptr = evt;
				epoll_ctl(SSockets_epollfd, EPOLL_CTL_ADD, conn, &ev);


				// Set up task's timerfd, for timeouts
				DISARM_TIMER(ctx->timerfd);
				ctx->timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

				// Create its event
				struct SSockets_event* newevt = malloc(sizeof(struct SSockets_event));
				newevt->type = SSockets_event_TIMEOUT;
				newevt->u.ctx = ctx;
				ctx->timerevt = newevt;

				// And listen to it
				ev.events = TIMER_EVENTS; // Recycling ev instance
				ev.data.ptr = newevt;
				epoll_ctl(SSockets_epollfd, EPOLL_CTL_ADD, ctx->timerfd, &ev);

				// Fallthrough to run the first task
			} else if(evt->type == SSockets_event_TIMEOUT) {
				// Something timed out
				ctx = evt->u.ctx;
				if(SSockets_timeoutCallback != NULL)
					SSockets_timeoutCallback(ctx);
				SSockets_closeAndDestroy(ctx);
				continue;
			} else {
				// Regular IO event
				ctx = evt->u.ctx;
			}

			if(ev & EPOLLHUP) {
				if(SSockets_hangupCallback != NULL)
					SSockets_hangupCallback(ctx);
				SSockets_closeAndDestroy(ctx);
				continue;
			}

			// If reached this point, the event is IN or OUT

			int result = SSockets_RET_OK;
			while(result == SSockets_RET_OK) {
				struct epoll_event ev;
				ev.events = BASIC_EVENTS;
				ev.data.ptr = evt;

				if(ctx->state >= SSockets_nTasks) {
					printf("SSockets: no state %lu\n", ctx->state);
					result = SSockets_RET_ERROR;
				} else {
					result = SSockets_tasks[ctx->state](ctx);
					if(ctx->disarm) {
						DISARM_TIMER(ctx->timerfd);
						ctx->disarm = 0;
					} else if(ctx->timeout) {
						ARM_TIMER(ctx->timerfd, ctx->timeout);
						ctx->timeout = 0;
					}
				}

				switch(result) {
				case SSockets_RET_OK:
					break;
				case SSockets_RET_READ:
					ev.events |= EPOLLIN;
					break;
				case SSockets_RET_WRITE:
					ev.events |= EPOLLOUT;
					break;
				default:
					printf("SSockets: IO returned %d\n", result);
					// fallthrough
				case SSockets_RET_ERROR:
					SSockets_closeAndDestroy(ctx);
					break;
				}

				if(result != SSockets_RET_OK && result != SSockets_RET_ERROR) {
					// Waiting for something, so let's add to epoll
					int r = epoll_ctl(SSockets_epollfd, EPOLL_CTL_MOD, ctx->fd, &ev);
					if(r == -1) {
						if(errno == ENOENT) {
							epoll_ctl(SSockets_epollfd, EPOLL_CTL_ADD, ctx->fd, &ev);
						} else {
							printf("SSockets: EPOLL_CTL_MOD failure: %d", errno);
							result = SSockets_RET_ERROR;
							SSockets_closeAndDestroy(ctx);
						}
					}
				}
			}
		}
	}

	return NULL;
}
