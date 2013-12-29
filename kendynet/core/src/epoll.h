#ifndef _EPOLL_H
#define _EPOLL_H

#include "poller.h"
#include "socket.h"
#include <stdint.h>

int32_t  epoll_init(poller_t);
int32_t  epoll_loop(poller_t,int32_t timeout);
int32_t  epoll_register(poller_t,socket_t);
int32_t  epoll_unregister(poller_t,socket_t);
int32_t  epoll_unregister_recv(poller_t,socket_t);
int32_t  epoll_unregister_send(poller_t,socket_t);
int32_t  epoll_wakeup(poller_t);
#endif
