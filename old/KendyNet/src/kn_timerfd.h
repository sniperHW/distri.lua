#ifndef _KN_TIMERFD_H
#define _KN_TIMERFD_H

#include "kn_fd.h"

typedef struct kn_timerfd{
	kn_fd                 base;
	//uint32_t              timeout;
}kn_timerfd,*kn_timerfd_t;

kn_fd_t kn_new_timerfd(uint32_t timeout);


#endif
