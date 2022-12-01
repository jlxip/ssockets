#ifndef SSOCKETS_LIB_WORKER_H
#define SSOCKETS_LIB_WORKER_H

#include <basics.h>

#define ARM_TIMER(t, x) {    \
    struct itimerspec spec; \
    spec.it_value.tv_sec = x; \
    spec.it_value.tv_nsec = 0; \
    spec.it_interval.tv_sec = 0; \
    spec.it_interval.tv_nsec = 0; \
    timerfd_settime(t, 0, &spec, NULL); \
}

#define DISARM_TIMER(t) ARM_TIMER(t, 0)

void* SSockets_worker(void*);
void SSockets_closeAndDestroy(struct SSockets_ctx* ctx);

#endif
