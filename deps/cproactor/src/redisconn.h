#ifndef _REDISCONN_H
#define _REDISCONN_H

#include "kn_fd.h"

//redis连接
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "kn_dlist.h"

struct kn_proactor;
enum{
	REDIS_CONNECTING = 1,
	REDIS_ESTABLISH,
	REDIS_CLOSE,
};

typedef struct redisconn{
	kn_fd                 base;
	int                   state;//REDIS_CONNECTING/REDIS_ESTABLISH
	redisAsyncContext*    context;
	void (*cb_connect)(struct redisconn*,int err);
	void (*cb_disconnected)(struct redisconn*);
	kn_dlist              pending_command;
	
}redisconn,*redisconn_t;

#endif
