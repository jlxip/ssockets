#include <basics.h>
#include <fcntl.h>

int nproc(void) {
	size_t a=11, b=0, c=1, d=0;
	asm volatile("cpuid"
				 : "=a" (a),
				   "=b" (b),
				   "=c" (c),
				   "=d" (d)
				 : "0" (a), "2" (c));
	return b;
}

int setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL);
	if(flags < 0)
		return 1;
	if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		return 1;
	return 0;
}
