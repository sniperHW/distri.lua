#ifndef _KN_STREAM_CONN_H
#define _KN_STREAM_CONN_H

typedef struct kn_stream_conn* kn_stream_conn_t;

void  kn_stream_conn_setud(kn_stream_conn_t,void *ud);
void *kn_stream_conn_getud(kn_stream_conn_t);

#endif