#ifndef _KN_STREAM_CONN_SERVER_H
#define _KN_STREAM_CONN_SERVER_H

#include "kendynet.h"
#include "kn_stream_conn.h"
#include "rpacket.h"
#include "kn_timer.h"

typedef struct kn_stream_server{
	kn_proactor_t proactor;
	void (*on_connection)(kn_stream_conn_t);
	void (*on_disconnected)(kn_stream_conn_t,int err);
	kn_fd_t      listen_fd;
	kn_sockaddr  server_addr;
	kn_timer_t   timer;
}kn_stream_server,*kn_stream_server_t;


/*
* serveraddr可以为空,为空表示不创建监听,可以创建一个stream_server用于监听,一组stream_server不监听.
* 监听server接受conn后将conn传递给其它stream_server处理连接
*/

kn_stream_server_t kn_new_stream_server(kn_proactor_t,
									    kn_sockaddr *serveraddr,
									    void (*)(kn_stream_conn_t),//on_connect
										void (*)(kn_stream_conn_t,int err)//on_disconnected		
									    );

void kn_destroy_stream_server(kn_stream_server_t);

void kn_stream_server_bind(kn_stream_server_t,
						   kn_stream_conn_t,
					       int(*)(kn_stream_conn_t,rpacket_t),//on packet
						   uint32_t recv_timeout,
						   void (*)(kn_stream_conn_t),//on recv timeout
						   uint32_t send_timeout,
						   void (*)(kn_stream_conn_t) //on send timeout									
						   );


#endif