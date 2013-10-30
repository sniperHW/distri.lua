#ifndef _EPOLL_H
#define _EPOLL_H

#include "Engine.h"
#include "Socket.h"
#include <stdint.h>

int32_t  epoll_init(engine_t);
int32_t  epoll_loop(engine_t,int32_t timeout);
int32_t  epoll_register(engine_t,socket_t);
int32_t  epoll_unregister(engine_t,socket_t);
#endif
