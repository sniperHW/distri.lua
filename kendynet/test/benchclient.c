#include <stdio.h>
#include <stdlib.h>
#include "core/netservice.h"
#include "testcommon.h"

uint32_t ava_interval = 0;

char *msg;
uint32_t send_size = 0;
int8_t on_process_packet(struct connection *c,rpacket_t r)
{
    //wpacket_t l_wpk = NEW_WPK(send_size);
    //wpk_write_binary(l_wpk,(void*)msg,send_size);
    //send_packet(c,l_wpk,NULL);
    send_packet(c,wpk_create_by_rpacket(r));
    return 1;
}

void on_connect(SOCK s,struct sockaddr_in *addr_remote, void *ud,int err)
{
    if(s != INVALID_SOCK){
        struct connection * con = new_conn(s,1);
        add_client(con);
        struct netservice *tcpclient = (struct netservice *)ud;
		tcpclient->bind(tcpclient,con,on_process_packet,remove_client
						,0,NULL,0,NULL);
        wpacket_t wpk = NEW_WPK(send_size);
        wpk_write_binary(wpk,(void*)msg,send_size);
        send_packet(con,wpk);
    }
}

void _sendpacket()
{
	uint32_t i = 0;
	for(; i < client_count; ++i){
		if(clients[i]){
            wpacket_t wpk = NEW_WPK(send_size);
            wpk_write_binary(wpk,(void*)msg,send_size);
            send_packet(clients[i],wpk);
		}
	}
}

int main(int argc,char **argv)
{
	setup_signal_handler();
	init_clients();
    InitNetSystem();
    struct netservice *tcpclient = new_service();
    int csize = atoi(argv[3]);
    send_size = atoi(argv[4]);
    msg = calloc(1,send_size);
    int i = 0;
    for(; i < csize; ++i){
        tcpclient->connect(tcpclient,argv[1],atoi(argv[2]),(void*)tcpclient,on_connect,10000);
	}
    //uint32_t tick,now;
    //tick = now = GetSystemMs();
	while(!stop){
        tcpclient->loop(tcpclient,0);
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
