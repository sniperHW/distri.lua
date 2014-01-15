#ifndef _ASYNNET_DEFINE_H
#define _ASYNNET_DEFINE_H

#define MAX_NETPOLLER 64         //最大POLLER数量

struct poller_st
{
    msgque_t         mq_in;          //用于接收从逻辑层过来的消息
    netservice*      netpoller;      //底层的poller
    thread_t         poller_thd;
    atomic_32_t      flag;
};

struct asynnet
{
    uint32_t  poller_count;
    struct poller_st      netpollers[MAX_NETPOLLER];
    atomic_32_t           flag;
};




#endif
