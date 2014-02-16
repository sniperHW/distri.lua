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
#ifndef _KENDYNET_H
#define _KENDYNET_H
#include <stdint.h>
#include "llist.h"
#include "common_define.h"

//定义系统支持的最大套接字数量
#define MAX_SOCKET 4096

typedef void* ENGINE;
#define INVALID_ENGINE NULL
typedef void* SOCK;
#define INVALID_SOCK NULL

#include "sock_util.h"

/*IO请求和完成队列使用的结构*/
typedef struct
{
    lnode      next;
	struct     iovec *iovec;
	int32_t    iovec_count;
}st_io;

typedef struct 
{
	char     ip[32];
	int32_t  port;
	union{
		uint64_t  usr_data;
		void     *usr_ptr;
	};
}connect_request;

//初始化网络系统
int32_t      InitNetSystem();

void   CleanNetSystem();
//IO请求完成时callback
typedef void (*CB_IOFINISH)(int32_t,st_io*,uint32_t err_code);
//连接关闭时,对所有未完成的请求执行的callback
//typedef void (*CB_CLEARPENDING)(st_io*);

extern void (*destroy_stio)(st_io*);

typedef void (*CB_ACCEPT)(SOCK,struct sockaddr_in*,void *ud);
typedef void (*CB_CONNECT)(SOCK,struct sockaddr_in*,void *ud,int err);

ENGINE   CreateEngine();
void     CloseEngine(ENGINE);
int32_t  EngineRun(ENGINE,int32_t timeout);
int32_t  Bind2Engine(ENGINE,SOCK,CB_IOFINISH);

//when you want to stop listen,CloseSocket(SOCK)
SOCK EListen(ENGINE,const char *ip,int32_t port,void *ud,CB_ACCEPT);

//if nonblock connect ms > 0
int32_t EConnect(ENGINE,const char *ip,int32_t port,void *ud,CB_CONNECT,uint32_t ms);

int32_t EWakeUp(ENGINE);


/* return:
*  0,  io pending
*  >0, bytes transfer
*  0<, socket disconnect or error
*/
int32_t Recv(SOCK,st_io*,uint32_t *err_code);
int32_t Send(SOCK,st_io*,uint32_t *err_code);

/*
* return:
* 0, success
* 0<,socket disconnect
*/

int32_t Post_Recv(SOCK,st_io*);
int32_t Post_Send(SOCK,st_io*);

#endif
