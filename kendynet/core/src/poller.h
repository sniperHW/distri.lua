#ifndef _POLLER_H
#define _POLLER_H

#include "sync.h"
#include "link_list.h"
#include "double_link.h"
#include <stdint.h>
#include "common.h"

struct socket_wrapper;

typedef struct poller
{
    int32_t  (*Init)(struct poller*);
    int32_t  (*Loop)(struct poller*,int32_t timeout);
    int32_t  (*Register)(struct poller*,struct socket_wrapper*);
    int32_t  (*UnRegister)(struct poller*,struct socket_wrapper*);
    int32_t  (*UnRegisterRecv)(struct poller*,struct socket_wrapper*);
    int32_t  (*UnRegisterSend)(struct poller*,struct socket_wrapper*);
    int32_t  (*WakeUp)(struct poller*);
	int32_t   poller_fd;
	struct    epoll_event events[MAX_SOCKET+1];
	int32_t   pipe_writer;//用于异步唤醒的管道
	int32_t   pipe_reader;
    struct    double_link actived[2];
    int8_t    actived_index;
	struct    double_link connecting;
}*poller_t;

poller_t poller_new();
void   poller_delete(poller_t *);

static inline struct double_link* get_active_list(poller_t e){
    return &e->actived[e->actived_index];
}

static inline int32_t is_active_empty(poller_t e)
{
    struct double_link *current_active = &e->actived[e->actived_index];
    return double_link_empty(current_active);
}

static inline void putin_active(poller_t e,struct double_link_node *dln)
{
    struct double_link *current_active = &e->actived[e->actived_index];
    double_link_push(current_active,dln);
}

#endif
