#ifndef _KN_EPOLL_H
#define _KN_EPOLL_H

#include "kn_proactor.h"
#include "kn_fd.h"

typedef struct kn_epoll{
	kn_proactor         base;
	int                 epfd;
	struct epoll_event* events;
	int                 eventsize;
	int                 maxevents;
}kn_epoll;



kn_epoll*kn_epoll_new();
void     kn_epoll_del(kn_epoll*);

int32_t  kn_epoll_loop(kn_proactor_t,int32_t timeout);
int32_t  kn_epoll_register(kn_proactor_t,kn_fd_t);
int32_t  kn_epoll_unregister(kn_proactor_t,kn_fd_t);
int32_t  kn_epoll_unregister_recv(kn_proactor_t,kn_fd_t);
int32_t  kn_epoll_unregister_send(kn_proactor_t,kn_fd_t);

#endif
