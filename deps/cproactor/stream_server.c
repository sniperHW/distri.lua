#include <stdio.h>
#include "kendynet.h"
#include "kn_stream_conn_server.h"
#include "kn_time.h"

uint32_t client_count;
uint32_t recvcount;

int  on_packet(kn_stream_conn_t conn,rpacket_t rpk){
	++recvcount;
	kn_stream_conn_send(conn,wpk_create_by_rpacket(rpk));
	kn_stream_conn_close(conn);
	return 1;
}

void on_disconnected(kn_stream_conn_t conn,int err){
	printf("on_disconnected\n");
	--client_count;
}

void new_client(kn_stream_server_t server,kn_stream_conn_t conn){
	printf("new_client\n");
	++client_count;
	kn_stream_server_bind(server,conn,1,1024,on_packet,on_disconnected,
						  10*1000,NULL,0,NULL);
}

int main(int argc,char **argv)
{
	kn_proactor_t p = kn_new_proactor();
	kn_sockaddr local;
	kn_addr_init_in(&local,argv[1],atoi(argv[2]));		
	kn_stream_server_t s = kn_new_stream_server(p,&local,new_client);
	uint64_t tick,now;
    tick = now = kn_systemms64();	
	while(1){
		kn_proactor_run(p,50);
        now = kn_systemms64();
		if(now - tick > 1000)
		{
            uint32_t elapse = (uint32_t)(now-tick);
            recvcount = (recvcount/elapse)*1000;
			printf("client_count:%d,recvcount:%d/s,buffer_count:%u\n",client_count,recvcount,buffer_count);
			tick = now;
			recvcount = 0;
		}		
	}	
	return 0;
}
