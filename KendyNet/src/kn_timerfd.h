#ifndef _KN_TIMERFD_H
#define _KN_TIMERFD_H

#include "kn_type.h"

typedef struct kn_timerfd{
	struct st_head comm_head;
}kn_timerfd,*kn_timerfd_t;

handle_t kn_new_timerfd(uint32_t timeout);


#endif
