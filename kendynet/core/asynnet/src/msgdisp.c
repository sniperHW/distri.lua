#include "../msgdisp.h"
#include "asynsock.h"
#include "asynnet_define.h"
#include "db/asyndb.h"
#include "../asyncall.h"
#include <stdarg.h>

void new_connection(SOCK sock,struct sockaddr_in *addr_remote,void *ud);

void msg_destroyer(void *ud)
{
    msg_t msg = (msg_t)ud;
    if(MSG_TYPE(msg) == MSG_RPACKET){
        rpacket_t rpk = (rpacket_t)msg;
        rpk_destroy(&rpk);
    }
    else if(MSG_TYPE(msg) == MSG_WPACKET){
        wpacket_t rpk = (wpacket_t)msg;
        wpk_destroy(&rpk);
    }else if(MSG_TYPE(msg) == MSG_DB_RESULT)
        free_dbresult((db_result_t)msg);
    else
        free(msg);
}


int32_t asynnet_connect(msgdisp_t disp,int32_t pollerid,const char *ip,int32_t port,uint32_t timeout)
{
    asynnet_t asynet = disp->asynet;
    struct msg_connect *msg = calloc(1,sizeof(*msg));
    msg->base.type = MSG_CONNECT;
    strcpy(msg->ip,ip);
    msg->port = port;
    msg->timeout = timeout;
    MSG_USRPTR(msg) = (void*)disp->mq;

    if(pollerid <= 0 || pollerid > asynet->poller_count)
        pollerid = 0;
    else
        pollerid -= 1;

    if(0 != msgque_put_immeda(asynet->netpollers[pollerid].mq_in,(lnode*)msg)){
        msg_destroyer((msg_t)msg);
        return -1;
    }
    return 0;
}

int32_t asynnet_bind(msgdisp_t disp,int32_t pollerid,sock_ident sock,int8_t raw,uint32_t recv_timeout,uint32_t send_timeout)
{
    asynsock_t asysock = cast_2_asynsock(sock);
    if(!asysock) return -1;
    struct msg_bind *msg = calloc(1,sizeof(*msg));
    msg->base.type = MSG_BIND;
    msg->base._ident = TO_IDENT(sock);
    msg->recv_timeout = recv_timeout;
    msg->send_timeout = send_timeout;
    msg->raw = raw;
    asysock->que = disp->mq;
    asynnet_t asynet = disp->asynet;
    int32_t idx = 0;//当poller_count>1时,netpollers[0]只用于监听和connect
    if(pollerid <= 0 || pollerid > asynet->poller_count){
        if(asynet->poller_count > 1) idx = rand()%(asynet->poller_count-1) + 1;
    }else
        idx = pollerid-1;
    if(0 != msgque_put_immeda(asynet->netpollers[idx].mq_in,(lnode*)msg)){
        msg_destroyer((msg_t)msg);
        asynsock_release(asysock);
        return -1;
    }
    asynsock_release(asysock);
    return 0;
}


sock_ident asynnet_listen(msgdisp_t disp,int32_t pollerid,const char *ip,int32_t port,int32_t *reason)
{
    sock_ident ret = {make_empty_ident()};
    asynnet_t asynet = disp->asynet;

    if(pollerid <= 0 || pollerid > asynet->poller_count)
        pollerid = 0;
    else
        pollerid -= 1;

    netservice *netpoller = asynet->netpollers[pollerid].netpoller;
    SOCK s = netpoller->listen(netpoller,ip,port,(void*)disp->mq,new_connection);
    if(s != INVALID_SOCK)
    {
        asynsock_t asysock = asynsock_new(NULL,s);
        asysock->sndque = asynet->netpollers[pollerid].mq_in;
        return asysock->sident;
    }else
    {
        *reason = errno;
        return ret;
    }
}

msgdisp_t  new_msgdisp(asynnet_t asynet,uint8_t cbsize,...)
{
    msgdisp_t disp = calloc(1,sizeof(*disp));
    disp->asynet = asynet;
    disp->mq = new_msgque(64,msg_destroyer);
    void *asyncb[TYPE_SIZE] = {NULL};
    if(cbsize){
        va_list argptr;
        va_start(argptr,cbsize);
        while(cbsize){
            uint8_t type = (uint8_t)va_arg(argptr,uint32_t);
            if(type >= TYPE_SIZE)
            {
                printf("invaild asyncb type\n");
                abort();
            }
            if(asyncb[type])
                abort();
            asyncb[type] = va_arg(argptr,void*);
            --cbsize;
        }
    }
    disp->on_connect = (ASYNCB_CONNECT)asyncb[TYPE_CONNECT];
    disp->on_connected = (ASYNCB_CONNECTED)asyncb[TYPE_CONNECTED];
    disp->on_disconnect = (ASYNCB_DISCNT)asyncb[TYPE_DISCNT];
    disp->process_packet = (ASYNCB_PROCESS_PACKET)asyncb[TYPE_PROCESS_PACKET];
    disp->connect_failed = (ASYNCN_CONNECT_FAILED)asyncb[TYPE_CONNECT_FAILED];
    disp->bind = asynnet_bind;
    disp->connect = asynnet_connect;
    disp->listen = asynnet_listen;
    return disp;
}


static void dispatch_msg(msgdisp_t disp,msg_t msg)
{
    //printf("dispatch_msg\n");
    if(msg->type == MSG_RPACKET)
    {
        //printf("RPACKET\n");
        rpacket_t rpk = (rpacket_t)msg;
        if(disp->process_packet && 0 == disp->process_packet(disp,rpk))
            return;
    }else if(msg->type == MSG_DB_RESULT){
        db_result_t result = (db_result_t)msg;
        if(result->callback) result->callback(result);
    }else if(msg->type == MSG_ASYNCALL){
        msg_asyncall_t asyncall = (msg_asyncall_t)msg;
        if(asyncall->fn_call){
            asyncall->fn_call(asyncall->context,(void**)&asyncall->param);
        }
    }else if(msg->type == MSG_ASYNRESULT){
        msg_asynresult_t asynresult = (msg_asynresult_t)msg;
        if(asynresult->context->fn_result)
           asynresult->context->fn_result(asynresult->context,asynresult->context->result);
    }
    else{
        struct msg_connection *tmsg = (struct msg_connection*)msg;
        //printf("dispatch_msg %d\n",msg->type);
        sock_ident sock = TO_SOCK(tmsg->base._ident);
        if(msg->type == MSG_ONCONNECT){
            //printf("MSG_ONCONNECT\n");
            if(disp->on_connect)
                disp->on_connect(disp,sock,tmsg->ip,tmsg->port);
            else
                asynsock_close(sock);
        }
        else if(msg->type == MSG_ONCONNECTED){
             //printf("MSG_ONCONNECTED\n");
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
    }
    msg_destroyer(msg);
}

void msg_loop(msgdisp_t disp,uint32_t ms)
{
    uint64_t nowtick = GetSystemMs64();
    uint64_t timeout = nowtick+(uint64_t)ms;
    do{
        msg_t _msg = NULL;
        uint32_t sleeptime = (uint32_t)(timeout - nowtick);
        msgque_get(disp->mq,(lnode**)&_msg,sleeptime);
        if(_msg) dispatch_msg(disp,_msg);
        nowtick = GetSystemMs64();
    }while(nowtick < timeout);
}

int32_t send_msg(msgdisp_t sender,msgdisp_t recver,msg_t msg)
{
    if(!msg || !recver || recver == sender)
        return -1;
    if(sender)
        (*(msgsender*)&MSG_IDENT(msg)) = make_by_msgdisp(sender);
    int32_t ret = msgque_put_immeda(recver->mq,(lnode*)msg);
    if(ret != 0)
        msg_destroyer(msg);
    return ret;
}

