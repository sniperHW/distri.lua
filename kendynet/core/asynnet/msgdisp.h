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
#ifndef _MSGDISP_H
#define _MSGDISP_H


/*
 *   消息分离器
 *   使用模式1:多线程共享一个消息分离器(多线程共用一个网络到逻辑消息队列)
 *   使用模式2:各线程使用单独的消息分离器(各线程有独立的网络到逻辑消息队列)
 *
*/


#include "asynnet.h"
#include "common_define.h"
typedef struct msgdisp* msgdisp_t;


/*
 * 用于标识一个rpacket到底是来自外部网络连接还是进程内部的其它msgdisp_t
 * 如果来自套接口msgsender就是一个sock_ident,否则标识了来源msgdisp_t
*/
typedef struct msgsender{
    ident _ident;
}msgsender;


#define TO_MSGSENDER(IDENT) (*(msgsender *)&IDENT)

static inline msgsender make_by_msgdisp(msgdisp_t disp)
{
    ident _ident;
    ((uint16_t*)&_ident.identity)[2] = type_msgdisp;
    _ident.ptr = (void*)disp;
    return *((msgsender*)&_ident);
}

static inline msgdisp_t get_msgdisp(msgsender sender)
{
    if(!is_type(TO_IDENT(sender),type_msgdisp)) return NULL;
    return (msgdisp_t)sender._ident.ptr;
}


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
typedef int32_t (*ASYNCB_PROCESS_PACKET)(msgdisp_t,rpacket_t);


typedef void (*ASYNCN_CONNECT_FAILED)(msgdisp_t,const char *ip,int32_t port,uint32_t reason);


enum{
    TYPE_CONNECT = 0,
    TYPE_CONNECTED,
    TYPE_DISCNT,          
    TYPE_PROCESS_PACKET,
    TYPE_CONNECT_FAILED,
    TYPE_SIZE,    
};


typedef struct msgdisp{
    msgque_t  mq;          //用于接收从网络过来的消息
    asynnet_t asynet;
    ASYNCB_CONNECT         on_connect;
    ASYNCB_CONNECTED       on_connected;
    ASYNCB_DISCNT          on_disconnect;
    ASYNCB_PROCESS_PACKET  process_packet;
    ASYNCN_CONNECT_FAILED  connect_failed;
    /*
    *    param:pollerid,如果填<=0则由系统来选择poller,否则使用用户传入的pollerid,pollerid<=asynnet创建时创建的pollercount
    */
    int32_t    (*bind)(msgdisp_t,int32_t pollerid,sock_ident,int8_t israw,uint32_t recvtimeout,uint32_t sendtimeout);

    /*
    *   param:pollerid,如果填<=0默认用第1个poller
    */
    sock_ident (*listen)(msgdisp_t,int32_t pollerid,const char*,int32_t,int32_t*);
    int32_t    (*connect)(msgdisp_t,int32_t pollerid,const char*,int32_t,uint32_t);

}*msgdisp_t;

static inline void* cb_asynconnect(ASYNCB_CONNECT fn){return (void*)fn;}
#define CB_CONNECT(FN_CB) (uint32_t)TYPE_CONNECT,cb_asynconnect(FN_CB)

static inline void* cb_asynconnected(ASYNCB_CONNECTED fn){return (void*)fn;}
#define CB_CONNECTED(FN_CB) (uint32_t)TYPE_CONNECTED,cb_asynconnected(FN_CB)

static inline void* cb_asyndiscnt(ASYNCB_DISCNT fn){return (void*)fn;}
#define CB_DISCNT(FN_CB) (uint32_t)TYPE_DISCNT,cb_asyndiscnt(FN_CB)

static inline void* cb_asynprocesspacket(ASYNCB_PROCESS_PACKET fn){return (void*)fn;}
#define CB_PROCESSPACKET(FN_CB) (uint32_t)TYPE_PROCESS_PACKET,cb_asynprocesspacket(FN_CB)

static inline void* cb_asynconnectfailed(ASYNCN_CONNECT_FAILED fn){return (void*)fn;}
#define CB_CONNECTFAILED(FN_CB) (uint32_t)TYPE_CONNECT_FAILED,cb_asynconnectfailed(FN_CB)

msgdisp_t  new_msgdisp(asynnet_t,uint8_t cbsize,...);

void       msg_loop(msgdisp_t,uint32_t timeout);

//直接往msgdisp_t的消息队列中投递消息,如果sender不为NULL,则将msg->_ident设为sender
int32_t    send_msg(msgdisp_t sender,msgdisp_t recver,msg_t msg);



#endif
