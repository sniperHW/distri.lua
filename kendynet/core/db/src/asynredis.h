#ifndef _ASYNREDIS_H
#define _ASYNREDIS_H

#include "../asyndb.h"
#include "msgque.h"
#include "thread.h"
#include "hiredis.h"
#include "llist.h"
#include "sync.h"

struct redis_worker
{
	lnode           node;
	thread_t        worker;
	volatile int8_t stop; 
	redisContext   *context;
	msgque_t        mq;
	char            ip[32];
	int32_t         port;
};

struct asynredis
{
	struct asyndb base;
	mutex_t mtx;
	struct llist  workers;
	msgque_t   mq;
};

int32_t redis_connectdb(asyndb_t,const char *ip,int32_t port);
int32_t redis_request(asyndb_t,db_request_t);

#endif