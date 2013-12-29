#include <stdio.h>
#include <stdlib.h>
#include "core/kendynet.h"
#include "core/connection.h"
#include "core/systime.h"
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
        bind2engine((ENGINE)ud,con,on_process_packet,remove_client);
    }
}

int main(int argc,char **argv)
{
    mutil_thread = 0;
	setup_signal_handler();
	init_clients();
    InitNetSystem();
    ENGINE engine = CreateEngine();
    int csize = atoi(argv[3]);
    int i = 0;
    for(; i < csize; ++i)
        EConnect(engine,argv[1],atoi(argv[2]),(void*)engine,on_connect,1000);
    uint32_t tick,now;
    tick = now = GetSystemMs();
	while(!stop){
		EngineRun(engine,50);
		sendpacket();
        now = GetSystemMs();
		if(now - tick > 1000)
		{
			printf("ava_interval:%d\n",ava_interval);
			tick = now;
		}
	}
    CleanNetSystem();
    return 0;
}
