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
#include "double_link.h"
#include "KendyNet.h"

enum{
	DATA = 1,
	LISTEN = 2,
	CONNECT = 3,
};

typedef struct socket_wrapper
{
	struct double_link_node dnode;
	volatile int32_t status;       //0:未分配,1:正常
	volatile int32_t  readable;
	volatile int32_t  writeable;
	volatile uint32_t stamp;
	struct engine  *engine;
	int32_t fd;
	struct link_list pending_send;//尚未处理的发请求
	struct link_list pending_recv;//尚未处理的读请求
    int8_t  socket_type;           //DATA or ACCEPTOR
	union{
	    //for data socket
        struct{
            void (*io_finish)(int32_t,st_io*,uint32_t err_code);
            void (*clear_pending_io)(st_io*);
        };
        //for listen or Connecting socket
        struct {
            uint32_t timeout;
            void *ud;
            SOCK  sock;
            union{
                void (*on_accept)(SOCK,void*);
                void (*on_connect)(SOCK,void*,int);
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

struct socket_wrapper *get_socket_wrapper(SOCK s);
int32_t get_fd(SOCK s);

#endif
