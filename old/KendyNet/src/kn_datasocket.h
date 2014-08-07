#ifndef _KN_DATASOCKET_H
#define _KN_DATASOCKET_H

#include "kn_fd.h"
/*
* 数据传输套接口的基类
*/

typedef struct kn_datasocket{
	kn_fd                 base;
	kn_list               pending_send;//尚未处理的发请求
    kn_list               pending_recv;//尚未处理的读请求
    struct kn_sockaddr    addr_local;
    struct kn_sockaddr    addr_remote;
    uint8_t               flag;
	uint8_t               connected;
	//请求完成后回调
    void   (*cb_transfer)(kn_fd_t,st_io*,int32_t bytestransfer,int32_t err);
    //销毁时处理pending中的剩余元素
    void   (*destroy_stio)(st_io*);
    int32_t (*raw_send)(struct kn_datasocket*,st_io *io_req,int32_t *err_code);
    int32_t (*raw_recv)(struct kn_datasocket*,st_io *io_req,int32_t *err_code);	
}kn_datasocket,*kn_datasocket_t;

//void    kn_datasocket_on_active(kn_fd_t s,int event);
//int8_t  kn_datasocket_process(kn_fd_t s);
//void    kn_datasocket_destroy(void *ptr);

kn_fd_t kn_new_datasocket(int fd,int sock_type,struct kn_sockaddr *local,struct kn_sockaddr *remote);

#endif
