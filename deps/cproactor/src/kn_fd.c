#include "kn_fd.h"

int  kn_set_noblock(kn_fd_t s){
	int ret;
	ret = fcntl(s->fd, F_SETFL, O_NONBLOCK | O_RDWR);
	if (ret != 0) {
		return -errno;
	}
	return 0;
}


