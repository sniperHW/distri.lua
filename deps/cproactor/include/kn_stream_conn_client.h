#ifndef _KN_STREAM_CONN_CLIENT_H
#define _KN_STREAM_CONN_CLIENT_H

#include "kendynet.h"
#include "kn_stream_conn.h"
#include "rpacket.h"
#include "kn_timer.h"

typedef struct kn_stream_client* kn_stream_client_t;

kn_stream_client_t kn_new_stream_client(kn_proactor_t,
										void (*)(kn_stream_client_t,kn_stream_conn_t),//on_connect
										void (*)(kn_stream_client_t,kn_sockaddr*,int err)//on_connect_fail
										);
										
void kn_destroy_stream_client(kn_stream_client_t);

int  kn_stream_client_bind(kn_stream_client_t,
						   kn_stream_conn_t,
						   int8_t   is_raw,
						   uint32_t recvbuf_size,
					       int(*)(kn_stream_conn_t,rpacket_t),//on packet
					       void (*)(kn_stream_conn_t,int err),//on disconnect
						   uint32_t recv_timeout,
						   void (*)(kn_stream_conn_t),//on recv timeout
						   uint32_t send_timeout,
						   void (*)(kn_stream_conn_t) //on send timeout									
						   );
						   
int kn_stream_connect(kn_stream_client_t,
					  struct kn_sockaddr *addr_local,				  
			          struct kn_sockaddr *addr_remote,
			          uint64_t timeout);				   

#endif
