/*
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _SOCKETWRAPPER_H
#define _SOCKETWRAPPER_H
#include <stdint.h>
#include "dlist.h"
#include "kendynet.h"
#include "refbase.h"

enum{
	DATA = 1,
	LISTEN = 2,
	CONNECT = 3,
};

enum{
	SCLOSE = 0,
	SESTABLISH = 1 << 1,  //双向通信正常
	SWCLOSE = 1 << 2,     //写关闭
	SRCLOSE = 1 << 3,     //读关闭
	SWAITCLOSE = 1 << 4,  //用户主动关闭，等发送队列中数据发完关闭
};

typedef struct socket_wrapper
{
    struct dnode       node;
    struct refbase     ref;
    volatile uint32_t  status;
	volatile int32_t  readable;
	volatile int32_t  writeable;
    struct poller     *engine;
	int32_t fd;
    struct llist      pending_send;//尚未处理的发请求
    struct llist      pending_recv;//尚未处理的读请求
    int8_t  socket_type;           //DATA or ACCEPTOR
    struct sockaddr_in addr_local;
    struct sockaddr_in addr_remote;
    union{
	    //for data socket
        void (*io_finish)(int32_t,st_io*,uint32_t err_code);
        //for listen or Connecting socket
        struct {
            uint64_t timeout;
            void *ud;
            SOCK  sock;
            union{
                void (*on_accept)(SOCK,struct sockaddr_in*,void*);
                void (*on_connect)(SOCK,struct sockaddr_in*,void*,int);
            };
        };
	};
}*socket_t;


void     on_read_active(socket_t);
void     on_write_active(socket_t);
void     process_connect(socket_t);
void     process_accept(socket_t);
int32_t  Process(socket_t);

int32_t raw_send(socket_t s,st_io *io_req,uint32_t *err_code);
int32_t raw_recv(socket_t s,st_io *io_req,uint32_t *err_code);


SOCK new_socket_wrapper();
void acquire_socket_wrapper(SOCK s);
void release_socket_wrapper(SOCK s);
struct socket_wrapper *get_socket_wrapper(SOCK s);
int32_t get_fd(SOCK s);

void   shutdown_recv(socket_t s);
void   shutdown_send(socket_t s);
void   clear_pending_send(socket_t s);
void   clear_pending_recv(socket_t s);

static inline int8_t test_sendable(uint32_t status){
	if(status == 0 || status & SWCLOSE)
		return 0;
	return 1;
}

static inline int8_t test_recvable(uint32_t status){
	if(status == 0 || status & SRCLOSE)
		return 0;
	return 1;
}

static inline int8_t test_waitclose(uint32_t status){
	return status & SWAITCLOSE;
}

#endif
