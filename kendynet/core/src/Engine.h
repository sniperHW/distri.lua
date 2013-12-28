#ifndef _ENGINE_H
#define _ENGINE_H

#include "sync.h"
#include "link_list.h"
#include "double_link.h"
#include <stdint.h>
#include "common.h"

struct socket_wrapper;

typedef struct engine
{
	int32_t  (*Init)(struct engine*);
	int32_t  (*Loop)(struct engine*,int32_t timeout);
	int32_t  (*Register)(struct engine*,struct socket_wrapper*);
	int32_t  (*UnRegister)(struct engine*,struct socket_wrapper*);
    int32_t  (*UnRegisterRecv)(struct engine*,struct socket_wrapper*);
    int32_t  (*UnRegisterSend)(struct engine*,struct socket_wrapper*);
	int32_t  (*WakeUp)(struct engine*);
	int32_t   poller_fd;
	struct    epoll_event events[MAX_SOCKET+1];
	int32_t   pipe_writer;//用于异步唤醒的管道
	int32_t   pipe_reader;
	struct    double_link actived;
	struct    double_link connecting;
}*engine_t;

engine_t create_engine();
void   free_engine(engine_t *);

#endif
