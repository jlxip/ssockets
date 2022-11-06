#include "tasks.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_LEN 1024

int initTask(struct SSockets_ctx* ctx) {
	if(ctx->data == NULL) {
		ctx->data = malloc(sizeof(struct Data));
		struct Data* data = ctx->data;
		data->buffer = (char*)malloc(1024);
		data->ctr = 0;
	}

	ctx->state = STATE_READ;
	return SSockets_RET_OK;
}

int readTask(struct SSockets_ctx* ctx) {
	struct Data* data = ctx->data;
	int r = recv(ctx->fd, data->buffer + data->ctr, MAX_LEN - data->ctr, 0);
	if(r == 0) {
		return SSockets_RET_FINISHED;
	} else if(r < 0) {
		if(errno == EAGAIN)
			return SSockets_RET_READ;
		return SSockets_RET_ERROR;
	}

	data->ctr += r;
	if(data->buffer[data->ctr - 1] == '\n' || data->ctr == MAX_LEN) {
		// Got the whole line
		ctx->state = STATE_WRITE;
	} else {
		// Not yet the whole line
	}

	return SSockets_RET_OK;
}

int writeTask(struct SSockets_ctx* ctx) {
	struct Data* data = ctx->data;
	int r = send(ctx->fd, data->buffer, data->ctr, 0);
	if(r < 0) {
		if(errno == EAGAIN)
			return SSockets_RET_WRITE;
		return SSockets_RET_ERROR;
	}

	data->ctr -= r;
	if(data->ctr == 0) {
		// All sent
		ctx->state = STATE_READ;
	} else {
		// Not yet the whole buffer
	}

	return SSockets_RET_OK;
}
