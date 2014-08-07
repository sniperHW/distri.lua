#ifndef _KN_STREAM_CONN_H
#define _KN_STREAM_CONN_H

#include "wpacket.h"
#include "kn_ref.h"

typedef struct kn_stream_conn* kn_stream_conn_t;

void  kn_stream_conn_setud(kn_stream_conn_t,void *ud);
void *kn_stream_conn_getud(kn_stream_conn_t);
void kn_stream_conn_close(kn_stream_conn_t c);
int32_t kn_stream_conn_send(kn_stream_conn_t c,wpacket_t w);

kn_sockaddr*  kn_stream_conn_local_addr(kn_stream_conn_t);

kn_sockaddr*  kn_stream_conn_remote_addr(kn_stream_conn_t);

ident   kn_stream_conn_makeident(kn_stream_conn_t c);
//禁止在kn_stream_conn_t的持有线程以外调用
kn_stream_conn_t cast2_kn_stream_conn(ident);


#endif
