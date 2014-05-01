#ifndef _KENDYNET_H
#define _KENDYNET_H

#include "kn_common_define.h"
#include "kn_sockaddr.h"

typedef struct kn_proactor* kn_proactor_t;
typedef struct kn_socket*   kn_socket_t;

extern  int kn_max_proactor_fd; 

int32_t kn_net_open();
void    kn_net_clean();

kn_proactor_t kn_new_proactor();
void          kn_close_proactor(kn_proactor_t);

kn_socket_t kn_connect(int protocol,int type,struct kn_sockaddr *addr_local,struct kn_sockaddr *addr_remote);

//异步connect,只对SOCK_STREAM有效
int kn_asyn_connect(struct kn_proactor*,
			   int protocol,
			   int type,
			   struct kn_sockaddr *addr_local,				  
			   struct kn_sockaddr *addr_remote,
			   void (*)(kn_socket_t,struct kn_sockaddr*,void*,int),
			   void *ud,
			   uint64_t timeout);


kn_socket_t kn_listen(struct kn_proactor*,
					  int protocol,
					  int type,
					  kn_sockaddr *addr_local,
					  void (*)(kn_socket_t,void*),
					  void *ud);

typedef void (*kn_cb_transfer)(kn_socket_t,st_io*,int32_t bytestransfer,int32_t err);
					  
kn_socket_t kn_dgram_listen(struct kn_proactor*,
					        int protocol,
					        int type,
					        kn_sockaddr *addr_local,
					        kn_cb_transfer);					  

int32_t       kn_proactor_run(kn_proactor_t,int32_t timeout);

int32_t       kn_proactor_bind(kn_proactor_t,kn_socket_t,kn_cb_transfer);


void          kn_closesocket(kn_socket_t);
void          kn_shutdown_recv(kn_socket_t);
void          kn_shutdown_send(kn_socket_t);
int           kn_set_nodelay(kn_socket_t);
int           kn_set_noblock(kn_socket_t);

int32_t       kn_recv(kn_socket_t,st_io*,uint32_t *err_code);
int32_t 	  kn_send(kn_socket_t,st_io*,uint32_t *err_code);

int32_t 	  kn_post_recv(kn_socket_t,st_io*);
int32_t 	  kn_post_send(kn_socket_t,st_io*);

//uint8_t       kn_socket_getstatus(kn_socket_t);

kn_proactor_t kn_socket_getproactor(kn_socket_t);

void          kn_socket_setud(kn_socket_t,void*);

void*         kn_socket_getud(kn_socket_t);

int           kn_socket_set_recvbuf(kn_socket_t,int size);

int           kn_socket_set_sendbuf(kn_socket_t,int size);

int           kn_socket_get_type(kn_socket_t);

kn_sockaddr*  kn_socket_get_local_addr(kn_socket_t);

//设置销毁函数，用于在kn_socket_t被销毁时销毁pending list中的st_io
typedef       void   (*stio_destroyer)(st_io*);
void          kn_socket_set_stio_destroyer(kn_socket_t,stio_destroyer);


#endif
