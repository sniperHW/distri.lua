#ifndef _KN_TYPE
#define _KN_TYPE
#include "kendynet.h"

struct st_head{
	int type;
	int status;
	int fd;
	void *ud;
	void (*on_events)(handle_t,int events);
};


enum{
	KN_SOCKET = 1,
	KN_TIMERFD,
	KN_FILE,
};

enum{
	kn_none = 0,
	kn_establish,
	kn_connecting,
	kn_listening,
	kn_close,
};

#endif
