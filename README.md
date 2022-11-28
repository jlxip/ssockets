# SSockets

## Introduction
SSockets (_Scalable Sockets_) is a C library (works in C++ too) for high-performance TCP servers. It handles worker threads and epoll events in an uncomplicated way so that you can avoid most of the C non-blocking socket boilerplate.

There is now [an awesome Python wrapper](https://github.com/jlxip/pyssockets) for SSockets.

## Why your socket servers are wrong
If you already know about `epoll`, you can skip this section and jump to [The basics](#the-basics).

Have you ever written a TCP client? Doesn't matter the language. In most of the cases, you only handle one connection at the time. You try to `connect()`, and the function returns when it suceeds or an error occurred. Same applies to `send()` and `recv()`.

What about servers? Have you ever written an HTTP server? Maybe FTP? A custom protocol for a CTF challenge? I'd assume the point is to handle multiple connections at the same time. How do you do it? You could `accept()` the requests and spawn a new thread for each one (_accept-and-fork_). This is slow since both `fork()` and `clone()` are expensive, which severly limits your throughput.

You should be using worker threads instead, but how do you orchestrate them? A naïve approach would be to `accept()` requests on a thread, and send the file descriptor to one of the workers which will take care of it for the rest of the connection. This puts an early limit on the number of simultaneous connections: the number of threads you spawned.

You should be using non-blocking sockets. [Here](https://www.scottklement.com/rpg/socktut/nonblocking.html)'s a gentle introduction to them. With non-blocking sockets, you multiplex connections so that you can run different parts of the requests in a non-sequential way. All I/O wait is used for CPU-intensive parts. How do you balance these _parts_ throughout the threads? You might think, as I once thought, that the best approach is to give a full request to one of the threads, in order to maximize L1 cache hits. However, since network latency is way greater than cache miss penalties, the best approach is to use a [combined queue](https://youtu.be/F5Ri_HhziI0): balance the parts, not the requests, across threads.

You could do this, as I once did, manually: `accept()` a connection in a main thread, and add their parts to a global pool of _things that need to be done_. Worker threads take one out and manage it. This consumes a lot of CPU power, since you must always be checking for all pending I/O operations. Also, you'd only be `accept()`ing requests in one thread, something that, when many clients are trying to connect at the same time, limits your throughput.

You must wait for events asynchronously in order to not be checking for I/O FIFO-style all the time. Historically, there were [`select()`](https://man7.org/linux/man-pages/man2/select.2.html), and later [`poll()`](https://man7.org/linux/man-pages/man2/poll.2.html). Both of them have plenty of issues and should not be used nowadays. You can read Marek's posts under the series [_I/O multiplexing_](https://idea.popcount.org/2016-11-01-a-brief-history-of-select2/) for more information. The correct answer is to use `epoll`, which might be confusing at first since it has many pitfalls. It's easy to end up with race conditions.

I was writing [zodiac](https://github.com/jlxip/zodiac) and [bsgemini](https://github.com/jlxip/bsgemini) using `epoll` when I realized that my way of organizing the I/O events was susceptible to being a library, which would shave most of the `epoll` boilerplate off both projects. SSockets is the result.

## The basics
I define a _job_ as a whole connection, from `accept()` to `close()`. In order to take advantage I/O multiplexing, a _job_ is divided in _tasks_, that is, states in the protocol you're implementing.

When a request comes, task 0 is ran. You jump from state to state in order to tell SSockets which is the next task to run. If your protocol is simple, you probably just have to increment the state identifier.

This requires very little code to work in most applications. You can even set timeouts and handle async events (for instance, _connection closed_).

## Installation
- Arch Linux: `ssockets` in the AUR
- Others: just `make && sudo make install`

## How to use
You might want to [see an example](https://github.com/jlxip/ssockets/tree/master/examples/echo/src).

You must model your tasks as different functions. They receive a pointer to a struct [`SSockets_ctx`](https://github.com/jlxip/ssockets/blob/master/pub/ssockets.h) and return an integer:
- `SSockets_RET_OK`, the task has finished successfully.
- `SSockets_RET_READ`, the task is pending input data.
- `SSockets_RET_WRITE`, the task is pending output data (in case the write buffer is full).
- `SSockets_RET_ERROR` or `SSockets_RET_FINISHED` (they're the same), when connection must be closed.

You add states with `SSockets_addState()`, which receives the function pointer and returns the state ID (monotonic, starts at zero). You can `enum` your way out of handling the return value; it's fine, no surprises.

Inside of the task function, you can access and alter some values of interest from the `SSockets_ctx` pointer:
- `state : size_t`. Contains the ID of the next task to run. You must change it if you don't intend to repeat the current one.
- `fd : int`. The file descriptor (probably socket) that caused the event. Don't touch it if you don't know what you're doing †.
- `addr : sockaddr_in`. The address that `accept()` filled with the connection details.
- `timeout : int`. When the task is executed, its value is zero. Change it to enable a timer in case you want to enable timeouts.
- `disarm : int`. Same as above. Set it to non-zero to disarm the timer; for example, in case you want to time out only one part of the job, such as reading the request, to avoid RUDY attacks.
- `data : void*`. Set this field to whatever you heart desires the most. Useful for keeping extra information regarding the connection, such as buffers.

You can set callbacks for some async events that might happen out of your control. These receive a `SSockets_ctx` pointer too, but return void. Set the callbacks with the following functions:
- `SSockets_setHangupCallback()`, for when the client closes the connection.
- `SSockets_setTimeoutCallback()`, for when a timeout occurs.
- `SSockets_setDestroyCallback()`, in case you need to free manually allocated memory, such as `ctx->data` if you set it earlier.

All these callbacks are called just before the data structures are freed. Because they will be: in case of timeout, connection will be closed.

When you're all set, call `SSockets_run()`. It receives three arguments:
- `addr : const char*`. The address to listen on. If you want all interfaces, you should set `0.0.0.0`.
- `port : uint16_t`. The port to listen on.
- `nthreads : size_t`. The number of worker threads to spawn. Set to zero to use as many as threads are available in the CPU. Do not use more: it will throttle.

† It's possible to manage multiple file descriptors by changing the `fd` field in the `SSockets_ctx` pointer, which might be useful if you're writing a proxy. In this case, when the task (or any subsequent) returns a wait value (either `SSockets_RET_READ` or `SSockets_RET_WRITE`), that file descriptor will be added to the epoll pool. Do keep in mind that you then need to manually keep track of all file descriptors you use via the `data` field. On the destroy callback, you must delete all of them from the `epoll` descriptor (`extern int SSockets_epollfd;`) and only then close them all.

## Additional information
- SSockets follows [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html)
- Dependencies: Linux >= 4.5

## Things built with SSockets
In case you need more examples, here are some projects of mine that are based on SSockets:
- [zodiac](https://github.com/jlxip/zodiac), a Gemini reverse proxy and load balancer.
- [bsgemini](https://github.com/jlxip/bsgemini), a zodiac backend for static servers.
