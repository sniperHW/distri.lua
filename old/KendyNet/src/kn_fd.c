#include "kn_fd.h"

void kn_set_noblock(int fd,int block){
    int flag = fcntl(fd, F_GETFL, 0);
    if (block) {
        flag &= (~O_NONBLOCK);
    } else {
        flag |= O_NONBLOCK;
    }
    fcntl(fd, F_SETFL, flag);
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


