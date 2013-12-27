#ifndef _CORONET_H
#define _CORONET_H

#include "netservice.h"
#include "thread.h"
#include "msg_que.h"
#include "datasocket.h"


typedef struct msg
{
	list_node next;
	uint8_t   type;
	union{
		uint64_t  usr_data;
		void*     usr_ptr;
		ident     _ident;
	};
	typedef void (*msg_destroy_function)();
}*msg_t;


struct engine_struct
{
	msgque_t     mq_in;          //用于接收从逻辑层过来的消息 
	netservice   netpoller;      //底层的poller
	thread_t     thread_engine;
	atomic_32_t  flag;
};

#define MAX_NETPOLLER 64         //最大POLLER数量

/* 连接成功后回调，此时连接还未绑定到通信poller,不可收发消息
*  用户可以选择直接关闭传进来的连接，或者将连接绑定到通信poller
*/

struct coronet;
typedef void (*coro_on_connect)(struct coronet*,ident);               

/*
*  绑定到通信poller成功后回调用，此时连接可正常收发消息
*/
typedef void (*coro_on_connected)(ident);

/*
*  连接断开后回调用
*/
typedef void (*coro_on_disconnect)(ident,uint32_t err);


/*
*  网络消息回调
*/
typedef void (*coro_process_packet)(ident,rpacket_t);


typedef void (*coro_connect_failed)(connect_request *creq,uint32_t);

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
	pthread_key_t       t_key;
	uint32_t            coro_count;
	atomic_32_t         flag;
}*coronet_t;

coronet_t create_coronet(uint8_t  pollercount,
	                     uint32_t coro_count,   //协程池的大小
	                     coro_on_connect,
						 coro_on_connected,
						 coro_on_disconnect,
						 coro_connect_failed,
						 coro_process_packet);


void      stop_coronet(coronet_t);

void      destroy_coronet(coronet_t);

int32_t   coronet_bind(coronet_t,ident sock,int8_t raw,uint32_t send_timeout,uint32_t recv_timeout);

int32_t   coronet_listen(coronet_t,const char*,int32_t);

int32_t   coronet_connect(coronet_t,connect_request *creq,uint32_t);

void      peek_msg(coronet_t,uint32_t timeout);

#endif