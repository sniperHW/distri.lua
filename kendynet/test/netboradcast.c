#include <stdio.h>
#include <stdlib.h>
#include "core/netservice.h"
#include "testcommon.h"


void on_process_packet(struct connection *c,rpacket_t r)
{
	send2_all_client(r);
}

void accept_client(SOCK s,void*ud)
{
	struct connection *c = new_conn(s,0);
	add_client(c);
	struct netservice *tcpserver = (struct netservice *)ud;
	tcpserver->bind(tcpserver,c,on_process_packet,remove_client
					,5000,c_recv_timeout,5000,c_send_timeout
					);
}

int main(int argc,char **argv)
{
    mutil_thread = 0;
	setup_signal_handler();
	init_clients();
    InitNetSystem();
    struct netservice *tcpserver = new_service();
	tcpserver->listen(tcpserver,argv[1],atoi(argv[2]),(void*)tcpserver,accept_client);
	uint32_t tick,now;
    tick = now = GetSystemMs();
	while(!stop){
		tcpserver->loop(tcpserver,50);
        now = GetSystemMs();
		if(now - tick > 1000)
		{
			printf("client_count:%d,send_count:%d\n",client_count,(packet_send_count*1000)/(now-tick));
			tick = now;
			packet_send_count = 0;
		}
	}
	destroy_service(&tcpserver);
    CleanNetSystem();
    return 0;
}

