#ifndef _MSGDISP_H
#define _MSGDISP_H


/*
 *   消息分离器
 *   使用模式1:多线程共享一个消息分离器(多线程共用一个网络到逻辑消息队列)
 *   使用模式2:各线程使用单独的消息分离器(各线程有独立的网络到逻辑消息队列)
 *
*/


#include "asynnet.h"

typedef struct msgdisp* msgdisp_t;

/* 连接成功后回调，此时连接还未绑定到通信poller,不可收发消息
*  用户可以选择直接关闭传进来的连接，或者将连接绑定到通信poller
*/

typedef void (*ASYNCB_CONNECT)(msgdisp_t,sock_ident,const char *ip,int32_t port);

/*
*  绑定到通信poller成功后回调用，此时连接可正常收发消息
*/
typedef void (*ASYNCB_CONNECTED)(msgdisp_t,sock_ident,const char *ip,int32_t port);

/*
*  连接断开后回调用
*/
typedef void (*ASYNCB_DISCNT)(msgdisp_t,sock_ident,const char *ip,int32_t port,uint32_t err);


/*
*   返回1：coro_process_packet调用后rpacket_t自动销毁
*   否则,将由使用者自己销毁
*/
typedef int32_t (*ASYNCB_PROCESS_PACKET)(msgdisp_t,sock_ident,rpacket_t);


typedef void (*ASYNCN_CONNECT_FAILED)(msgdisp_t,const char *ip,int32_t port,uint32_t reason);


typedef struct msgdisp{
    msgque_t  mq;          //用于接收从网络过来的消息
    asynnet_t asynet;
    ASYNCB_CONNECT         on_connect;
    ASYNCB_CONNECTED       on_connected;
    ASYNCB_DISCNT          on_disconnect;
    ASYNCB_PROCESS_PACKET  process_packet;
    ASYNCN_CONNECT_FAILED  connect_failed;
    int32_t    (*bind)(msgdisp_t,sock_ident,int8_t,uint32_t,uint32_t);
    sock_ident (*listen)(msgdisp_t,const char*,int32_t,int32_t*);
    int32_t    (*connect)(msgdisp_t,const char*,int32_t,uint32_t);


}*msgdisp_t;

msgdisp_t  new_msgdisp(asynnet_t,ASYNCB_CONNECT,
                                 ASYNCB_CONNECTED,
                                 ASYNCB_DISCNT,
                                 ASYNCB_PROCESS_PACKET,
                                 ASYNCN_CONNECT_FAILED);

void       msg_loop(msgdisp_t,uint32_t timeout);



#endif
