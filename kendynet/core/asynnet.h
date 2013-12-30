/*
*   多线程异步网络框架，将逻辑层与网络层分离
*   两层之间通过消息队列通信
*/

#ifndef _ASYNNET_H
#define _ASYNNET_H

#include "netservice.h"
#include "thread.h"
#include "msgque.h"

typedef struct sock_ident{
	ident _ident;
}sock_ident;

#define CAST_2_SOCK(IDENT) (*(sock_ident*)&IDENT)

typedef struct msg
{
    lnode next;
	uint8_t   type;
	union{
		uint64_t  usr_data;
		void*     usr_ptr;
		ident     _ident;
	};
	void (*msg_destroy_function)();
}*msg_t;

typedef struct asynnet* asynnet_t;

#define MAX_NETPOLLER 64         //最大POLLER数量

/* 连接成功后回调，此时连接还未绑定到通信poller,不可收发消息
*  用户可以选择直接关闭传进来的连接，或者将连接绑定到通信poller
*/

typedef void (*ASYNCB_CONNECT)(asynnet_t,sock_ident,const char *ip,int32_t port);

/*
*  绑定到通信poller成功后回调用，此时连接可正常收发消息
*/
typedef void (*ASYNCB_CONNECTED)(asynnet_t,sock_ident,const char *ip,int32_t port);

/*
*  连接断开后回调用
*/
typedef void (*ASYNCB_DISCNT)(asynnet_t,sock_ident,const char *ip,int32_t port,uint32_t err);


/*
*   返回1：coro_process_packet调用后rpacket_t自动销毁
*   否则,将由使用者自己销毁
*/
typedef int32_t (*ASYNCB_PROCESS_PACKET)(asynnet_t,sock_ident,rpacket_t);


typedef void (*ASYNCN_CONNECT_FAILED)(asynnet_t,const char *ip,int32_t port,uint32_t reason);

/*
*  listen的回调，如果成功返回一个合法的sock_ident
*  否则reason说明失败原因
*/
typedef void (*ASYNCB_LISTEN)(asynnet_t,sock_ident,const char *ip,int32_t port,uint32_t reason);

asynnet_t asynnet_new(uint8_t  pollercount,
                      ASYNCB_CONNECT,
                      ASYNCB_CONNECTED,
                      ASYNCB_DISCNT,
                      ASYNCB_PROCESS_PACKET,
                      ASYNCN_CONNECT_FAILED,
                      ASYNCB_LISTEN);


void      asynnet_stop(asynnet_t);

void      asynnet_coronet(asynnet_t);

int32_t   asynnet_bind(asynnet_t,sock_ident sock,void *ud,int8_t raw,uint32_t send_timeout,uint32_t recv_timeout);

int32_t   asynnet_listen(asynnet_t,const char *ip,int32_t port);

int32_t   asynnet_connect(asynnet_t,const char *ip,int32_t port,uint32_t timeout);

void      peek_msg(asynnet_t,uint32_t timeout);

int32_t   asynsock_close(sock_ident);

int32_t   get_addr_local(sock_ident,char *buf,uint32_t buflen);
int32_t   get_addr_remote(sock_ident,char *buf,uint32_t buflen);

int32_t   get_port_local(sock_ident,int32_t *port);
int32_t   get_port_remote(sock_ident,int32_t *port);

#endif
