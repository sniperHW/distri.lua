#include <stdio.h>
#include <stdlib.h>
#include "core/netservice.h"
#include "testcommon.h"

uint32_t ava_interval = 0;

void on_process_packet(struct connection *c,rpacket_t r)
{
	uint32_t s = rpk_read_uint32(r);
	uint32_t t;
	if(s == (uint32_t)c)
	{
		t = rpk_read_uint32(r);
		uint32_t sys_t = GetSystemMs();
		ava_interval += (sys_t - t);
		ava_interval /= 2;
	}
}



void on_connect(SOCK s,void*ud,int err)
{
    if(s != INVALID_SOCK){
        struct connection * con = new_conn(s,0);
        add_client(con);
		struct netservice *tcpclient = (struct netservice *)ud;
		tcpclient->bind(tcpclient,con,on_process_packet,remove_client
						,5000,c_recv_timeout,5000,c_send_timeout);
    }
}

int main(int argc,char **argv)
{
    mutil_thread = 0;
	setup_signal_handler();
	init_clients();
    InitNetSystem();
    struct netservice *tcpclient = new_service();
    int csize = atoi(argv[3]);
    int i = 0;
    for(; i < csize; ++i)
        tcpclient->connect(tcpclient,argv[1],atoi(argv[2]),(void*)tcpclient,on_connect,1000);
    uint32_t tick,now;
    tick = now = GetSystemMs();
	while(!stop){
		tcpclient->loop(tcpclient,50);
		sendpacket();
        now = GetSystemMs();
		if(now - tick > 1000)
		{
			printf("ava_interval:%d\n",ava_interval);
			tick = now;
		}
	}
	destroy_service(&tcpclient);
    CleanNetSystem();
    return 0;
}
