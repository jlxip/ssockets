#ifndef SSOCKETS_LIB_H
#define SSOCKETS_LIB_H

#include <stddef.h>
#include <stdint.h>
#include <arpa/inet.h>

struct SSockets_ctx {
	size_t state;
	int fd;
	struct sockaddr_in addr;
	int timerfd;

	// These are to be freed later in closeAndDestroy()
	void* regularevt;
	void* timerevt;

	// User can change these
	int timeout;
	int disarm;
	void* data;
};

typedef int (*SSockets_task)(struct SSockets_ctx*);
typedef void (*SSockets_callback)(struct SSockets_ctx*);

#ifdef __cplusplus
extern "C" {
#endif
	int SSockets_addState(SSockets_task cpu);

	void SSockets_setHangupCallback(SSockets_callback cb);
	void SSockets_setTimeoutCallback(SSockets_callback cb);
	void SSockets_setDestroyCallback(SSockets_callback cb);

	void SSockets_run(const char* ip, uint16_t port, size_t nthreads);
#ifdef __cplusplus
}
#endif

#define SSockets_RET_OK        0
#define SSockets_RET_READ     -1
#define SSockets_RET_WRITE    -2
#define SSockets_RET_ERROR    -3
#define SSockets_RET_FINISHED -3
// ↑ Those last two are intentionally the same

#endif
