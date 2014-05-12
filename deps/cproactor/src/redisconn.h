#ifndef _REDISCONN_H
#define _REDISCONN_H

#include "kn_fd.h"

//redis连接
#include "hiredis/hiredis.h"
#include "hiredis/async.h"

struct kn_proactor;
enum{
	REDIS_CONNECTING = 1,
	REDIS_ESTABLISH
};

typedef struct redisconn{
	kn_fd                 base;
	int                   state;//REDIS_CONNECTING/REDIS_ESTABLISH
	redisAsyncContext*    context;
	void (*cb_connect)(struct redisconn*,int err);
	
}redisconn,*redisconn_t;


/*
typedef struct redisLibeventEvents {
    redisAsyncContext *context;
    struct event rev, wev;
} redisLibeventEvents;

static void redisLibeventReadEvent(int fd, short event, void *arg) {
    ((void)fd); ((void)event);
    redisLibeventEvents *e = (redisLibeventEvents*)arg;
    redisAsyncHandleRead(e->context);
}

static void redisLibeventWriteEvent(int fd, short event, void *arg) {
    ((void)fd); ((void)event);
    redisLibeventEvents *e = (redisLibeventEvents*)arg;
    redisAsyncHandleWrite(e->context);
}

static void redisLibeventAddRead(void *privdata) {
    redisLibeventEvents *e = (redisLibeventEvents*)privdata;
    event_add(&e->rev,NULL);
}

static void redisLibeventDelRead(void *privdata) {
    redisLibeventEvents *e = (redisLibeventEvents*)privdata;
    event_del(&e->rev);
}

static void redisLibeventAddWrite(void *privdata) {
    redisLibeventEvents *e = (redisLibeventEvents*)privdata;
    event_add(&e->wev,NULL);
}

static void redisLibeventDelWrite(void *privdata) {
    redisLibeventEvents *e = (redisLibeventEvents*)privdata;
    event_del(&e->wev);
}

static void redisLibeventCleanup(void *privdata) {
    redisLibeventEvents *e = (redisLibeventEvents*)privdata;
    event_del(&e->rev);
    event_del(&e->wev);
    free(e);
}

static int redisLibeventAttach(redisAsyncContext *ac, struct event_base *base) {
    redisContext *c = &(ac->c);
    redisLibeventEvents *e;

    if (ac->ev.data != NULL)
        return REDIS_ERR;


    e = (redisLibeventEvents*)malloc(sizeof(*e));
    e->context = ac;


    ac->ev.addRead = redisLibeventAddRead;
    ac->ev.delRead = redisLibeventDelRead;
    ac->ev.addWrite = redisLibeventAddWrite;
    ac->ev.delWrite = redisLibeventDelWrite;
    ac->ev.cleanup = redisLibeventCleanup;
    ac->ev.data = e;

    event_set(&e->rev,c->fd,EV_READ,redisLibeventReadEvent,e);
    event_set(&e->wev,c->fd,EV_WRITE,redisLibeventWriteEvent,e);
    event_base_set(base,&e->rev);
    event_base_set(base,&e->wev);
    return REDIS_OK;
}
*/

#endif
