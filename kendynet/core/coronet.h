#ifndef _CORONET_H
#define _CORONET_H

#include "netservice.h"
#include "thread.h"
#include "msg_que.h"

typedef struct sock_ident{
	ident _ident;
}sock_ident;

#define CAST_2_SOCK(IDENT) (*(sock_ident*)&IDENT)

typedef struct msg
{
	list_node next;
	uint8_t   type;
	union{
		uint64_t  usr_data;
		void*     usr_ptr;
		ident     _ident;
	};
	void (*msg_destroy_function)();
}*msg_t;

struct coronet;
struct engine_struct
{
	msgque_t         mq_in;          //用于接收从逻辑层过来的消息 
	netservice*      netpoller;      //底层的poller
	thread_t         thread_engine;
	struct coronet*  _coronet;
	atomic_32_t  flag;
};

#define MAX_NETPOLLER 64         //最大POLLER数量

/* 连接成功后回调，此时连接还未绑定到通信poller,不可收发消息
*  用户可以选择直接关闭传进来的连接，或者将连接绑定到通信poller
*/

typedef void (*coro_on_connect)(struct coronet*,sock_ident,const char *ip,int32_t port);

/*
*  绑定到通信poller成功后回调用，此时连接可正常收发消息
*/
typedef void (*coro_on_connected)(struct coronet*,sock_ident,const char *ip,int32_t port);

/*
*  连接断开后回调用
*/
typedef void (*coro_on_disconnect)(struct coronet*,sock_ident,const char *ip,int32_t port,uint32_t err);


/*
*   返回1：coro_process_packet调用后rpacket_t自动销毁
*   否则,将由使用者自己销毁
*/
typedef int32_t (*coro_process_packet)(struct coronet*,sock_ident,rpacket_t);


typedef void (*coro_connect_failed)(struct coronet*,const char *ip,int32_t port,uint32_t reason);

/*
*  listen的回调，如果成功返回一个合法的sock_ident
*  否则reason说明失败原因
*/
typedef void (*coro_listen_ret)(struct coronet*,sock_ident,const char *ip,int32_t port,uint32_t reason);

typedef struct coronet
{
	uint32_t  poller_count;
	msgque_t  mq_out;                                 //用于向逻辑层发送消息
	struct engine_struct accptor_and_connector;       //监听器和连接器
	struct engine_struct netpollers[MAX_NETPOLLER];
	coro_on_connect     _coro_on_connect;
	coro_on_connected   _coro_on_connected;
	coro_on_disconnect  _coro_on_disconnect;
	coro_process_packet _coro_process_packet;
	coro_connect_failed _coro_connect_failed;
	coro_listen_ret     _coro_listen_ret;
	atomic_32_t         flag;
}*coronet_t;

coronet_t create_coronet(uint8_t  pollercount,
	                     coro_on_connect,
						 coro_on_connected,
						 coro_on_disconnect,
						 coro_connect_failed,
						 coro_listen_ret,
						 coro_process_packet);


void      stop_coronet(coronet_t);

void      destroy_coronet(coronet_t);

int32_t   coronet_bind(coronet_t,sock_ident sock,void *ud,int8_t raw,uint32_t send_timeout,uint32_t recv_timeout);

int32_t   coronet_listen(coronet_t,const char *ip,int32_t port);

int32_t   coronet_connect(coronet_t,const char *ip,int32_t port,uint32_t timeout);

void      peek_msg(coronet_t,uint32_t timeout);

int32_t   close_datasock(sock_ident);

int32_t   get_addr_local(sock_ident,char *buf,uint32_t buflen);
int32_t   get_addr_remote(sock_ident,char *buf,uint32_t buflen);

int32_t   get_port_local(sock_ident,int32_t *port);
int32_t   get_port_remote(sock_ident,int32_t *port);

#endif
