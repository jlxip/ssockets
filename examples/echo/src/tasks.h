#ifndef TASKS_H
#define TASKS_H

#include <ssockets.h>

struct Data {
	char* buffer;
	size_t ctr;
};

enum {
	STATE_INIT,
	STATE_READ,
	STATE_WRITE
};

int initTask(struct SSockets_ctx* ctx);
int readTask(struct SSockets_ctx* ctx);
int writeTask(struct SSockets_ctx* ctx);

#endif
