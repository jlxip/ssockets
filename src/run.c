#include <basics.h>
#include <signal.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <worker/worker.h>

#define MAX_CONNECTIONS 16

int SSockets_epollfd;

int nproc(void); // Auxiliary

void SSockets_run(const char* ip, uint16_t port, size_t nthreads) {
	// Ignore SIGPIPE, the standard practice in all TCP servers
	signal(SIGPIPE, SIG_IGN);

	// Set up the listening socket
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if(sock < 0) {
		perror("SSockets: unable to create listening socket\n");
		exit(EXIT_FAILURE);
	}

	const int enable = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		perror("SSockets: could not set SO_REUSEADDR\n");
		exit(EXIT_FAILURE);
	}

	if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("SSockets: could not bind to port\n");
		exit(EXIT_FAILURE);
	}

	if(listen(sock, MAX_CONNECTIONS) < 0) {
		perror("SSockets: could not listen\n");
		exit(EXIT_FAILURE);
	}

	// Build epoll file descriptor
	if((SSockets_epollfd = epoll_create1(0)) < 0) {
		perror("SSockets: could not create epoll fd\n");
		exit(EXIT_FAILURE);
	}

	// Spawn worker threads
	if(nthreads == 0)
		nthreads = nproc();
	for(size_t i=1; i<nthreads; ++i) {
		pthread_t thread;
		pthread_create(&thread, NULL, SSockets_worker, NULL);
	}

	// Add listening socket to epoll
	struct SSockets_event* evt = malloc(sizeof(struct SSockets_event));
	evt->type = SSockets_event_LISTEN;
	evt->u.listenfd = sock;
	struct epoll_event ev;
	ev.events = LISTEN_EVENTS;
	ev.data.ptr = evt;
	if(epoll_ctl(SSockets_epollfd, EPOLL_CTL_ADD, sock, &ev) == -1) {
		perror("SSockets: could not add listen socket to epoll\n");
		exit(EXIT_FAILURE);
	}

	SSockets_worker(NULL);
}
