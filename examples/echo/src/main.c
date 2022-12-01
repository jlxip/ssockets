#include <stdio.h>
#include <ssockets.h>
#include "tasks.h"
#include <stdlib.h>

// A very simple echo server using SSockets

void destroy(struct SSockets_ctx* ctx);

int main() {
    SSockets_addState(initTask);
    SSockets_addState(readTask);
    SSockets_addState(writeTask);
    SSockets_setDestroyCallback(destroy);
    SSockets_run("0.0.0.0", 4444, 0);
}

void destroy(struct SSockets_ctx* ctx) {
    struct Data* data = ctx->data;
    free(data->buffer);
    free(data);
}
