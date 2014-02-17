#include <stdio.h>
#include <stdlib.h>
#include "asynnet/msgdisp.h"
#include "testcommon.h"

uint32_t recvsize = 0;
uint32_t recvcount = 0;

void asynconnect(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port)
{
    printf("asynconnect,ip:%s,port:%d\n",ip,port);
    disp->bind(disp,0,sock,1,0,0);//由系统选择poller
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


int32_t asynprocesspacket(msgdisp_t disp,rpacket_t rpk)
{
    recvsize += rpk_len(rpk);
    recvcount++;
    asyn_send(TO_SOCK(MSG_IDENT(rpk)),wpk_create_by_rpacket(rpk));
    return 1;
}


void asynconnectfailed(msgdisp_t disp,const char *ip,int32_t port,uint32_t reason)
{

}


static const char *ip;
static int32_t port;

static void *service_main(void *ud){

    printf("echo service port:%d\n",port);
    msgdisp_t disp = (msgdisp_t)ud;
    int32_t err = 0;
    disp->listen(disp,0,ip,port++,&err);
    while(!stop){
        msg_loop(disp,500);
    }
    printf("service_main finish\n");
    return NULL;
}


int main(int argc,char **argv)
{
    //msgque_flush_time = 5;
    setup_signal_handler();
    InitNetSystem();

    //共用网络层，两个线程各运行一个echo服务

    asynnet_t asynet = asynnet_new(2);
    msgdisp_t  disp1 = new_msgdisp(asynet,5,
                                   CB_CONNECT(asynconnect),
                                   CB_CONNECTED(asynconnected),
                                   CB_DISCNT(asyndisconnected),
                                   CB_PROCESSPACKET(asynprocesspacket),
                                   CB_CONNECTFAILED(asynconnectfailed));

    /*msgdisp_t  disp2 = new_msgdisp(asynet,
                                  asynconnect,
                                  asynconnected,
                                  asyndisconnected,
                                  asynprocesspacket,
                                  asynconnectfailed);
    */
    thread_t service1 = create_thread(THREAD_JOINABLE);
    //thread_t service2 = create_thread(THREAD_JOINABLE);

    ip = argv[1];
    port = atoi(argv[2]);

    thread_start_run(service1,service_main,(void*)disp1);
    //sleepms(1000);
    //thread_start_run(service2,service_main,(void*)disp2);

    uint32_t tick,now;
    tick = now = GetSystemMs();
    while(!stop){
        sleepms(1000);
        now = GetSystemMs();
        //if(now - tick > 1000)
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

    thread_join(service1);
    //thread_join(service2);

    CleanNetSystem();
    return 0;
}
