/*
 *  一个网关示例
*/

#include <stdio.h>
#include <stdlib.h>
#include "core/msgdisp.h"
#include "testcommon.h"


static msgdisp_t  disp_to_server;
static msgdisp_t  disp_to_client;
sock_ident        to_server;


void to_server_connected(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port)
{
    to_server = sock;
}


int32_t to_client_process(msgdisp_t disp,msgsender sender,rpacket_t rpk)
{
    if(!eq_ident(TO_IDENT(sender),TO_IDENT(to_server))){
        //from cliet,send to server
        push_msg(disp,disp_to_server,(struct packet*)rpk);
    }else
    {
        //from server,send to client
        ident client = rpk_read_ident(rpk);
        asyn_send(CAST_2_SOCK(client),wpk_create_by_rpacket(rpk));
    }
    return 1;
}

void to_client_connect(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port)
{
    //用第3个poller处理到客户端的连接
    disp->bind(disp,3,sock,1,3*1000,0);//由系统选择poller
}


int32_t to_server_process(msgdisp_t disp,msgsender sender,rpacket_t rpk)
{
    if(!eq_ident(TO_IDENT(sender),TO_IDENT(to_server))){
        //from cliet,send to server        
        asyn_send(to_server,wpk_create_by_rpacket(rpk));
    }else{
        //from server,send to client
        push_msg(disp,disp_to_client,(struct packet*)rpk);
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

    msgdisp_t  disp_to_server = new_msgdisp(asynet,
                                  to_server_connect,
                                  to_server_connected,
                                  NULL,
                                  to_server_process,
                                  NULL);

    msgdisp_t  disp_to_client = new_msgdisp(asynet,
                                  to_client_connect,
                                  NULL,
                                  NULL,
                                  to_client_process,
                                  NULL);

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

/*
    广播包示例,场景服务器中的代码
    void BroadCast(wpacket_t wpk,sock_ident gate,sock_ident *client,uint16_t client_size)
    {
        int i = 0;
        for(; i < client_size; ++i)
            wpk_write_ident(wpk,TO_IDENT(client[i]));
        wpk_write_uint16(wpk,client_size);
        asyn_send(gate,wpk);
        wpacket_destroy(wpk);
    }
*/

/*
    网关中的代码
    uint16_t size = reverse_read_uint16(rpk);//这个包需要发给多少个客户端
    //在栈上创建一个rpacket_t用于读取需要广播的客户端
    rpacket_t r = rpk_stack_create(rpk,size*sizeof(sock_ident)+sizeof(size));
    //将rpk中用于广播的信息丢掉
    rpk_dropback(rpk,size*sizeof(sock_ident)+sizeof(size));
    int i = 0;
    wpacket_t wpk = wpk_create_by_rpacket(rpk);
    //发送给所有需要接收的客户端
    for( ; i < size; ++i)
    {
        ident _ident = rpk_read_ident(r);
        asyn_send(CAST_2_SOCK(_ident),wpk);
    }
    rpk_destroy(&r);
*/




