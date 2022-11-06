#include <basics.h>

size_t SSockets_nTasks = 0;
SSockets_task* SSockets_tasks = NULL;

int SSockets_addState(SSockets_task task) {
	int ret = SSockets_nTasks++;

	SSockets_tasks = realloc(SSockets_tasks,
							 SSockets_nTasks * sizeof(SSockets_task));

	SSockets_tasks[ret] = task;
	return ret;
}

SSockets_callback SSockets_hangupCallback = NULL;
void SSockets_setHangupCallback(SSockets_callback cb) {
	SSockets_hangupCallback = cb;
}

SSockets_callback SSockets_timeoutCallback = NULL;
void SSockets_setTimeoutCallback(SSockets_callback cb) {
	SSockets_timeoutCallback = cb;
}

SSockets_callback SSockets_destroyCallback = NULL;
void SSockets_setDestroyCallback(SSockets_callback cb) {
	SSockets_destroyCallback = cb;
}
