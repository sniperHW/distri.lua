#ifndef _KN_ACCEPTOR_H
#define _KN_ACCEPTOR_H

#include "kn_fd.h"

typedef struct kn_acceptor{
	kn_fd    base;
	kn_sockaddr  addr_local;
	void (*cb_accept)(kn_fd_t,void*);
}*kn_acceptor_t;


kn_acceptor_t kn_new_acceptor(int fd,kn_sockaddr *local,void (*)(kn_fd_t,void*),void *ud);

struct kn_proactor;





#endif
