#include <stdio.h>
#include <stdlib.h>
#include "core/kendynet.h"
#include "core/connection.h"
#include "core/systime.h"
#include "testcommon.h"

static int packet_send_count = 0;


void on_process_packet(struct connection *c,rpacket_t r)
{
	send2_all_client(r);
}

void accept_client(SOCK s,void*ud)
{
	struct connection *c = new_conn(s,0);
	add_client(c);
	bind2engine((ENGINE)ud,c,on_process_packet,remove_client);
}

int main(int argc,char **argv)
{
    mutil_thread = 0;
	setup_signal_handler();
	init_clients();
    InitNetSystem();
    ENGINE engine = CreateEngine();
    EListen(engine,argv[1],atoi(argv[2]),(void*)engine,accept_client);
    uint32_t tick,now;
    tick = now = GetSystemMs();
	while(!stop){
		EngineRun(engine,50);
        now = GetSystemMs();
		if(now - tick > 1000)
		{
			printf("client_count:%d,send_count:%d\n",client_count,(packet_send_count*1000)/(now-tick));
			tick = now;
			packet_send_count = 0;
		}
	}
    CleanNetSystem();
    return 0;
}
