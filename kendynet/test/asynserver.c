#include <stdio.h>
#include <stdlib.h>
#include "core/msgdisp.h"
#include "testcommon.h"

uint32_t recvsize = 0;
uint32_t recvcount = 0;
extern int disconnect_count;

///int32_t asynnet_bind(msgdisp_t disp,sock_ident sock,void *ud,int8_t raw,uint32_t send_timeout,uint32_t recv_timeout)
void asynconnect(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port)
{
    printf("asynconnect\n");
    disp->bind(disp,sock,0,30,0);
}

void asynconnected(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port)
{
    printf("asynconnected\n");
   ++client_count;
}

void asyndisconnected(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port,uint32_t err)
{
    --client_count;
}


int32_t asynprocesspacket(msgdisp_t disp,sock_ident sock,rpacket_t rpk)
{
    recvsize += rpk_len(rpk);
    recvcount++;
    asyn_send(sock,wpk_create_by_other((struct packet*)rpk));
    return 1;
}


void asynconnectfailed(msgdisp_t disp,const char *ip,int32_t port,uint32_t reason)
{

}


int main(int argc,char **argv)
{
    setup_signal_handler();
    InitNetSystem();
    asynnet_t asynet = asynnet_new(1);
    msgdisp_t  disp = new_msgdisp(asynet,
                                  asynconnect,
                                  asynconnected,
                                  asyndisconnected,
                                  asynprocesspacket,
                                  asynconnectfailed);
    int32_t err = 0;
    disp->listen(disp,argv[1],atoi(argv[2]),&err);
    uint32_t tick,now;
    tick = now = GetSystemMs();
    while(!stop){
        msg_loop(disp,50);
        now = GetSystemMs();
        if(now - tick > 1000)
        {
            uint32_t elapse = now-tick;
            recvsize = (recvsize/elapse)/1000;
            printf("client_count:%d,recvsize:%d,recvcount:%d,discount:%d\n",client_count,recvsize,recvcount,disconnect_count);
            tick = now;
            packet_send_count = 0;
            recvcount = 0;
            recvsize = 0;
        }
    }
    CleanNetSystem();
    return 0;
}
