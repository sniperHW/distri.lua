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
	int   (*associate)(engine_t,
			  handle_t,
			  void (*cb_ontranfnish)(handle_t,st_io*,int,int),
			  void (*destry_stio)(st_io*));		
	void (*on_events)(handle_t,int events);
	void (*on_destroy)(void*);
}handle;


enum{
	KN_SOCKET = 1,
	KN_TIMERFD = 2,
	KN_CHRDEV = 3,
	KN_REDISCONN = 4,
	KN_MAILBOX = 5,
};

#endif
