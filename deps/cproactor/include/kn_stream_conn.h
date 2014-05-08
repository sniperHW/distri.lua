#ifndef _KN_STREAM_CONN_H
#define _KN_STREAM_CONN_H

#include "wpacket.h"

typedef struct kn_stream_conn* kn_stream_conn_t;

void  kn_stream_conn_setud(kn_stream_conn_t,void *ud);
void *kn_stream_conn_getud(kn_stream_conn_t);
void kn_stream_conn_close(kn_stream_conn_t c);
int32_t kn_stream_conn_send(kn_stream_conn_t c,wpacket_t w);

#endif
