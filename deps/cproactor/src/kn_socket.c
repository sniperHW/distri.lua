#include "kn_socket.h"

int  kn_set_noblock(kn_socket_t s){
	int ret;
	ret = fcntl(s->fd, F_SETFL, O_NONBLOCK | O_RDWR);
	if (ret != 0) {
		return -errno;
	}
	return 0;
}


