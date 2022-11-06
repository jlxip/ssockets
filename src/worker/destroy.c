#include "worker.h"
#include <sys/epoll.h>
#include <unistd.h>

void SSockets_closeAndDestroy(struct SSockets_ctx* ctx) {
	if(SSockets_destroyCallback != NULL)
		SSockets_destroyCallback(ctx);

	epoll_ctl(SSockets_epollfd, EPOLL_CTL_DEL, ctx->fd, NULL);
	close(ctx->fd);

	epoll_ctl(SSockets_epollfd, EPOLL_CTL_DEL, ctx->timerfd, NULL);
	close(ctx->timerfd);

	free(ctx->regularevt);
	free(ctx->timerevt);
	free(ctx);
}
