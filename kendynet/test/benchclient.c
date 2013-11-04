#include <stdio.h>
#include <stdlib.h>
#include "core/netservice.h"
#include "testcommon.h"

uint32_t ava_interval = 0;

char msg[16000];
void on_process_packet(struct connection *c,rpacket_t r)
{
    wpacket_t l_wpk = NEW_WPK(16000);
    wpk_write_binary(l_wpk,(void*)msg,16000);
    send_packet(c,l_wpk,NULL);
}

void on_connect(SOCK s,void*ud,int err)
{
    if(s != INVALID_SOCK){
        struct connection * con = new_conn(s,0);
        add_client(con);
		struct netservice *tcpclient = (struct netservice *)ud;
		tcpclient->bind(tcpclient,con,on_process_packet,remove_client
						,0,NULL,0,NULL);
        wpacket_t wpk = NEW_WPK(16000);
        wpk_write_binary(wpk,(void*)msg,16000);
        send_packet(con,wpk,NULL);
    }
}

/*
char msg[4096];

void _sendpacket()
{
	uint32_t i = 0;
	for(; i < client_count; ++i){
		if(clients[i]){
            wpacket_t wpk = NEW_WPK(4096);
            wpk_write_binary(wpk,(void*)msg,4096);
            send_packet(clients[i],wpk,NULL);
		}
	}
}
*/

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
		tcpclient->loop(tcpclient,5);
		//_sendpacket();
        /*now = GetSystemMs();
		if(now - tick > 1000)
		{
			printf("ava_interval:%d\n",ava_interval);
			tick = now;
		}*/
	}
	destroy_service(&tcpclient);
    CleanNetSystem();
    return 0;
}
