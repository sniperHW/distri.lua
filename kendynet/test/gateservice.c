/*
 *  一个网关示例
*/

#include <stdio.h>
#include <stdlib.h>
#include "asynnet/msgdisp.h"
#include "testcommon.h"


static msgdisp_t  disp_to_server;
static msgdisp_t  disp_to_client;
sock_ident        to_server;


void to_server_connected(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port)
{
    to_server = sock;
}


int32_t to_client_process(msgdisp_t disp,rpacket_t rpk)
{
    if(!eq_ident(MSG_IDENT(rpk),TO_IDENT(to_server))){
        //from cliet,send to server
        send_msg(disp,disp_to_server,(msg_t)rpk);
    }else
    {
        //from server,send to client
        ident client = rpk_read_ident(rpk);
        asyn_send(TO_SOCK(client),wpk_create_by_rpacket(rpk));
    }
    return 1;
}

void to_client_connect(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port)
{
    //用第3个poller处理到客户端的连接
    disp->bind(disp,3,sock,1,3*1000,0);//由系统选择poller
}


int32_t to_server_process(msgdisp_t disp,rpacket_t rpk)
{
    if(!eq_ident(MSG_IDENT(rpk),TO_IDENT(to_server))){
        //from cliet,send to server        
        asyn_send(to_server,wpk_create_by_rpacket(rpk));
    }else{
        //from server,send to client
        send_msg(disp,disp_to_client,(msg_t)rpk);
    }
    return 1;
}

void to_server_connect(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port)
{
    //用第二个poller处理到服务器的连接
    disp->bind(disp,2,sock,1,3*1000,0);//由系统选择poller
}



//对客户端的监听
static const char *to_client_ip;
static int32_t to_client_port;


//对内部服务器的监听
static const char *to_server_ip;
static int32_t to_server_port;

static void *service_toclient(void *ud){
    msgdisp_t disp = (msgdisp_t)ud;
    int32_t err = 0;
    //用户的连接比较频繁,用一个单独的poller来处理监听
    disp->listen(disp,1,to_client_ip,to_client_port,&err);
    while(!stop){
        msg_loop(disp,50);
    }
    return NULL;
}

static void *service_toserver(void *ud){
    msgdisp_t disp = (msgdisp_t)ud;
    int32_t err = 0;
    disp->listen(disp,2,to_server_ip,to_server_port,&err);
    while(!stop){
        msg_loop(disp,50);
    }
    return NULL;
}



int main(int argc,char **argv)
{
    setup_signal_handler();
    InitNetSystem();

    asynnet_t asynet = asynnet_new(3);//3个poller,1个用于监听,1个用于处理客户端连接，1个用于处理服务器连接

    msgdisp_t  disp_to_server = new_msgdisp(asynet,3,
                                            CB_CONNECT(to_server_connect),
                                            CB_CONNECTED(to_server_connected),
                                            CB_PROCESSPACKET(to_server_process));

    msgdisp_t  disp_to_client = new_msgdisp(asynet,2,
                                            CB_CONNECT(to_client_connect),
                                            CB_ASYNPROCESSPACKET(to_client_process));

    thread_t service1 = create_thread(THREAD_JOINABLE);
    thread_t service2 = create_thread(THREAD_JOINABLE);

    to_client_ip = argv[1];
    to_client_port = atoi(argv[2]);


    to_server_ip = argv[3];
    to_server_port = atoi(argv[4]);

    thread_start_run(service1,service_toserver,(void*)disp_to_server);
    sleepms(1000);
    thread_start_run(service2,service_toclient,(void*)disp_to_client);

    while(!stop){
        sleepms(100);
    }

    thread_join(service1);
    thread_join(service2);

    CleanNetSystem();
    return 0;
}



