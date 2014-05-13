#include "kn_fd.h"

int  kn_set_noblock(kn_fd_t s){
	int ret;
	ret = fcntl(s->fd, F_SETFL, O_NONBLOCK | O_RDWR);
	if (ret != 0) {
		return -errno;
	}
	return 0;
}

kn_ref_destroyer kn_fd_get_destroyer(kn_fd_t fd){
	return fd->ref.destroyer;
}

void kn_fd_set_destroyer(kn_fd_t fd,void (*f)(void*)){
	fd->ref.destroyer = f; 
}

void kn_fd_addref(kn_fd_t fd){
	kn_ref_acquire(&fd->ref);
}

void kn_fd_subref(kn_fd_t fd){
	kn_ref_release(&fd->ref);
}


