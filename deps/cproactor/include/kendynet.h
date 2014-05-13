#ifndef _KENDYNET_H
#define _KENDYNET_H

#include "kn_common_define.h"
#include "kn_sockaddr.h"
#include "kn_ref.h"

typedef struct kn_proactor* kn_proactor_t;
typedef struct kn_fd*       kn_fd_t;
typedef ident               kn_channel_t;

extern  int kn_max_proactor_fd; 

int32_t kn_net_open();
void    kn_net_clean();

kn_proactor_t kn_new_proactor();
void          kn_close_proactor(kn_proactor_t);

kn_fd_t kn_connect(int type,struct kn_sockaddr *addr_local,struct kn_sockaddr *addr_remote);

//异步connect,只对SOCK_STREAM有效
int kn_asyn_connect(struct kn_proactor*,
			   int type,
			   struct kn_sockaddr *addr_local,				  
			   struct kn_sockaddr *addr_remote,
			   void (*)(kn_fd_t,struct kn_sockaddr*,void*,int),
			   void *ud,
			   uint64_t timeout);


kn_fd_t kn_listen(struct kn_proactor*,
					  //int protocol,
					  //int type,
					  kn_sockaddr *addr_local,
					  void (*)(kn_fd_t,void*),
					  void *ud);

typedef void (*kn_cb_transfer)(kn_fd_t,st_io*,int32_t bytestransfer,int32_t err);
					  
kn_fd_t kn_dgram_listen(struct kn_proactor*,
					        //int protocol,
					        //int type,
					        kn_sockaddr *addr_local,
					        kn_cb_transfer);					  

int32_t       kn_proactor_run(kn_proactor_t,int32_t timeout);

int32_t       kn_proactor_bind(kn_proactor_t,kn_fd_t,kn_cb_transfer);


void          kn_closefd(kn_fd_t);
void          kn_shutdown_recv(kn_fd_t);
void          kn_shutdown_send(kn_fd_t);
int           kn_set_nodelay(kn_fd_t);
int           kn_set_noblock(kn_fd_t);

int32_t       kn_recv(kn_fd_t,st_io*,int32_t *err_code);
int32_t 	  kn_send(kn_fd_t,st_io*,int32_t *err_code);

int32_t 	  kn_post_recv(kn_fd_t,st_io*);
int32_t 	  kn_post_send(kn_fd_t,st_io*);

//uint8_t       kn_fd_getstatus(kn_fd_t);

kn_proactor_t kn_fd_getproactor(kn_fd_t);

void          kn_fd_setud(kn_fd_t,void*);

void*         kn_fd_getud(kn_fd_t);

int           kn_fd_set_recvbuf(kn_fd_t,int size);

int           kn_fd_set_sendbuf(kn_fd_t,int size);

int           kn_fd_get_type(kn_fd_t);

kn_sockaddr*  kn_fd_get_local_addr(kn_fd_t);

/* 增加/减少fd的引用计数,典型应用场景
*  假设连接上收到两个顺序消息A,B,在A消息的回调中,保存了fd，并启动另一个异步操作
*  在异步操作的回调中向fd发送消息,在B消息的回调中调用kn_close(fd).在这个场景下
*  B消息的kn_close操作将导致fd的内存被释放.当A消息等待的回调调用时将会操作一个
*  非法的fd.所以,在启动异步操作前需要先调用kn_fd_addref,等回调完成后执行kn_fd_subref
*  这样将保证fd不会被错误的释放.
*/ 
void kn_fd_addref(kn_fd_t fd);
void kn_fd_subref(kn_fd_t fd);


//用于替换fd的默认销毁函数
kn_ref_destroyer kn_fd_get_destroyer(kn_fd_t);
void          kn_fd_set_destroyer(kn_fd_t,void (*)(void*));

//设置销毁函数，用于在kn_fd_t被销毁时销毁pending list中的st_io
typedef       void   (*stio_destroyer)(st_io*);
void          kn_fd_set_stio_destroyer(kn_fd_t,stio_destroyer);

kn_channel_t kn_new_channel(pthread_t owner);
void         kn_channel_close(kn_channel_t);

int          kn_channel_bind(struct kn_proactor*,kn_channel_t,
				             void(*)(kn_channel_t, kn_channel_t,void*,void*),
				             void*);
/*
*  from:如果不为空,表示如果要对这条消息作响应响应消息发往from. 
*/ 
int  kn_channel_putmsg(kn_channel_t to,kn_channel_t *from,void*);

typedef struct redisconn *redisconn_t;

int kn_redisAsynConnect(kn_proactor_t p,
						const char *ip,unsigned short port,
						void (*cb_connect)(redisconn_t,int err),
						void (*cb_disconnected)(struct redisconn*)
						);

struct redisReply;						
int kn_redisCommand(redisconn_t,const char *cmd,
					void (*cb)(redisconn_t,struct redisReply*,void *pridata),void *pridata);					

void kn_redisDisconnect(redisconn_t);

#endif
