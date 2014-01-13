#include "msgdisp.h"
#include "asynsock.h"


void new_connection(SOCK sock,struct sockaddr_in *addr_remote,void *ud);

int32_t asynnet_connect(msgdisp_t disp,const char *ip,int32_t port,uint32_t timeout)
{
    asynnet_t asynet = disp->asynet;
    struct msg_connect *msg = calloc(1,sizeof(*msg));
    msg->base.type = MSG_CONNECT;
    strcpy(msg->ip,ip);
    msg->port = port;
    msg->timeout = timeout;
    MSG_USRPTR(msg) = (void*)disp->mq;
    if(0 != msgque_put_immeda(asynet->netpollers[0].mq_in,(lnode*)msg)){
        free(msg);
        return -1;
    }
    return 0;
}

int32_t asynnet_bind(msgdisp_t disp,sock_ident sock,void *ud,int8_t raw,uint32_t send_timeout,uint32_t recv_timeout)
{
    asynsock_t asysock = cast_2_asynsock(sock);
    if(!asysock) return -1;
    struct msg_bind *msg = calloc(1,sizeof(*msg));
    msg->base.type = MSG_BIND;
    msg->base._ident = TO_IDENT(sock);
    msg->recv_timeout = recv_timeout;
    msg->send_timeout = send_timeout;
    msg->raw = raw;
    msg->ud =  ud;
    asysock->que = disp->mq;
    asynnet_t asynet = disp->asynet;
    int32_t idx = 0;//当poller_count>1时,netpollers[0]只用于监听和connect
    if(asynet->poller_count > 1) idx = rand()%(asynet->poller_count-1) + 1;
    if(0 != msgque_put_immeda(asynet->netpollers[idx].mq_in,(lnode*)msg)){
        free(msg);
        asynsock_release(asysock);
        return -1;
    }
    asynsock_release(asysock);
    return 0;
}


sock_ident asynnet_listen(msgdisp_t disp,const char *ip,int32_t port,int32_t *reason)
{
    sock_ident ret = {make_empty_ident()};
    asynnet_t asynet = disp->asynet;
    netservice *netpoller = asynet->netpollers[0].netpoller;
    SOCK s = netpoller->listen(netpoller,ip,port,disp,new_connection);
    if(s != INVALID_SOCK)
    {
        asynsock_t asysock = asynsock_new(NULL,s);
        asysock->sndque = asynet->netpollers[0].mq_in;
        return asysock->sident;
    }else
    {
        *reason = errno;
        return ret;
    }
}

void mq_item_destroyer(void *ptr);

msgdisp_t  new_msgdisp(asynnet_t asynet,
                       ASYNCB_CONNECT        on_connect,
                       ASYNCB_CONNECTED      on_connected,
                       ASYNCB_DISCNT         on_disconnect,
                       ASYNCB_PROCESS_PACKET process_packet,
                       ASYNCN_CONNECT_FAILED connect_failed)
{

    if(!asynet || !on_connect || !on_connected || !on_disconnect || !process_packet)
        return NULL;
    msgdisp_t disp = calloc(1,sizeof(*disp));
    disp->asynet = asynet;
    disp->mq = new_msgque(64,mq_item_destroyer);
    disp->on_connect = on_connect;
    disp->on_connected = on_connected;
    disp->on_disconnect = on_disconnect;
    disp->process_packet = process_packet;
    disp->connect_failed = connect_failed;
    return disp;
}


static void dispatch_msg(msgdisp_t disp,msg_t msg)
{
    if(msg->type == MSG_RPACKET)
    {
        rpacket_t rpk = (rpacket_t)msg;
        sock_ident sock = CAST_2_SOCK(MSG_IDENT(rpk));
        if(disp->process_packet(disp,sock,rpk))
            rpk_destroy(&rpk);
    }else{
        struct msg_connection *tmsg = (struct msg_connection*)msg;
        sock_ident sock = CAST_2_SOCK(tmsg->base._ident);
        if(msg->type == MSG_ONCONNECT){
            if(disp->on_connect)
                disp->on_connect(disp,sock,tmsg->ip,tmsg->port);
            else
                asynsock_close(sock);
        }
        else if(msg->type == MSG_ONCONNECTED){
            if(disp->on_connected)
                disp->on_connected(disp,sock,tmsg->ip,tmsg->port);
            else
                asynsock_close(sock);
        }
        else if(msg->type == MSG_DISCONNECTED && disp->on_disconnect){
                disp->on_disconnect(disp,sock,tmsg->ip,tmsg->port,tmsg->reason);
        }
        else if(msg->type == MSG_CONNECT_FAIL && disp->connect_failed)
            disp->connect_failed(disp,tmsg->ip,tmsg->port,tmsg->reason);
        free(msg);
    }
}

void msg_loop(msgdisp_t disp,uint32_t ms)
{
    uint32_t nowtick = GetSystemMs();
    uint32_t timeout = nowtick+ms;
    do{
        msg_t _msg = NULL;
        uint32_t sleeptime = timeout - nowtick;
        msgque_get(disp->mq,(lnode**)&_msg,sleeptime);
        if(_msg) dispatch_msg(disp,_msg);
        nowtick = GetSystemMs();
    }while(nowtick < timeout);
}

