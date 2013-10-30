#ifndef _NETSERVICE_H
#define _NETSERVICE_H
#include "KendyNet.h"
#include "Connection.h"
#include "SysTime.h"
#include "timing_wheel.h"

typedef struct netservice{
    ENGINE engine;
    TimingWheel_t timer;
    int32_t (*bind)(struct netservice *,struct connection*,process_packet,on_disconnect,
                    uint32_t,on_recv_timeout,uint32_t,on_send_timeout);
    SOCK    (*listen)(struct netservice*,const char*,int32_t,void*,OnAccept);
    int32_t (*connect)(struct netservice*,const char*,int32_t,void*,OnConnect,uint32_t);
    int32_t (*loop)(struct netservice*,uint32_t ms);
}netservice;

struct netservice *new_service(uint32_t precision,uint32_t max);//传入定时器相关参数
void   destroy_service(struct netservice**);


#endif
