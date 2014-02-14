#include <stdio.h>
#include <stdlib.h>
#include "core/netservice.h"
#include "testcommon.h"
#include "refbase.h"

uint32_t recvsize = 0;
uint32_t recvcount = 0;

int8_t on_process_packet(struct connection *c,rpacket_t r)
{
	//printf("pksize %d\n",rpk_len(r));
    recvsize += rpk_len(r);
    recvcount++;
    send_packet(c,wpk_create_by_rpacket(r));
	//active_close(c);
	//send2_all_client(r);
    return 1;
}

void client_come(struct connection *c)
{
    ++client_count;
}

void client_go(struct connection *c,uint32_t reason)
{
    --client_count;
	release_conn(c);
}

void accept_client(SOCK s,struct sockaddr_in *addr_remote,void*ud)
{
	struct connection *c = new_conn(s,1);
	client_come(c);
	struct netservice *tcpserver = (struct netservice *)ud;
	tcpserver->bind(tcpserver,c,on_process_packet,client_go,0,NULL,0,NULL);
}


int main(int argc,char **argv)
{
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
            uint32_t elapse = now-tick;
            recvsize = (recvsize/elapse)/1000;
			printf("client_count:%d,recvsize:%d,recvcount:%d\n",client_count,recvsize,recvcount);
			tick = now;
			packet_send_count = 0;
			recvcount = 0;
			recvsize = 0;
		}
	}
	destroy_service(&tcpserver);
    CleanNetSystem();
    return 0;
}

