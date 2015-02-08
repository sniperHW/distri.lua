#ifndef _KN_TYPE
#define _KN_TYPE
#include "kendynet_private.h"
#include "kn_list.h"

typedef struct handle{
	kn_list_node next;
	int type;
	int status;
	int fd;
	void *ud;
	union{
		int    events;
		struct{
			short set_read;
			short set_write;
		};
	};	
	void (*on_events)(handle_t,int events);
	void (*on_destroy)(void*);
}handle;


enum{
	KN_SOCKET = 1,
	KN_TIMERFD,
	KN_CHRDEV,
	KN_REDISCONN,
	KN_MAILBOX,
};

#endif
