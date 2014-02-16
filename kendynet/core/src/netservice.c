#include "netservice.h"
#include "socket.h"

void check_timeout(struct timer* t,struct timer_item *wit,void *ud)
{
    uint64_t now = GetSystemMs64();
    struct connection *c = wheelitem2con(wit);
    acquire_conn(c);
    if(test_recvable(c->status) && c->cb_recv_timeout && now > c->last_recv + (uint64_t)c->recv_timeout){
		printf("recv timeout\n");
        c->cb_recv_timeout(c);
	}
    if(test_sendable(c->status) && c->cb_send_timeout)
    {
        wpacket_t wpk = (wpacket_t)llist_head(&c->send_list);
        if(wpk && now > wpk->base.tstamp + (uint64_t)c->send_timeout){
			printf("send timeout\n");
            c->cb_send_timeout(c);
		}
    }
    if(c->status != SCLOSE) register_timer(t,wit,1);
    release_conn(c);
}

static int32_t _bind(struct netservice *n,
                     struct connection *c,
                     CCB_PROCESS_PKT cb_process_packet,
                     CCB_DISCONNECT cb_disconnect,
                     uint32_t rtimeout,
                     CCB_RECV_TIMEOUT cb_recv_timeout,
                     uint32_t stimeout,
                     CCB_SEND_TIMEOUT cb_send_timeout)
 {
    c->cb_recv_timeout = cb_recv_timeout;
    c->cb_send_timeout = cb_send_timeout;
    c->recv_timeout = rtimeout;
    c->send_timeout = stimeout;
    c->wheelitem.ud_ptr = (void*)n;
    c->wheelitem.callback = check_timeout;
    if(cb_recv_timeout || cb_send_timeout)
        register_timer(n->timer,con2wheelitem(c),1);
    return bind2engine(n->engine,c,cb_process_packet,cb_disconnect);
 }



static SOCK _listen(struct netservice *n,
                           const char *ip,int32_t port,void *ud,CB_ACCEPT on_accept)
{
  return EListen(n->engine,ip,port,ud,on_accept);
}


int32_t _connect(struct netservice *n,const char *ip,int32_t port,void *ud,CB_CONNECT on_connect,
                 uint32_t timeout)
{
    return EConnect(n->engine,ip,port,ud,on_connect,timeout);
}

int32_t _loop(struct netservice *n,uint32_t ms)
{
    update_timer(n->timer,GetSystemSec());
    return EngineRun(n->engine,ms);
}

int32_t _wakeup(struct netservice *n)
{
	return EWakeUp(n->engine);
}


struct netservice *new_service()
{
    struct netservice *n = calloc(1,sizeof(*n));
    n->timer = new_timer();
    n->engine = CreateEngine();
    if(n->engine == INVALID_ENGINE)
    {
        delete_timer(&n->timer);
        free(n);
        return NULL;
    }
    n->bind = _bind;
    n->listen = _listen;
    n->loop = _loop;
    n->connect = _connect;
	n->wakeup = _wakeup;
    return n;
}

void destroy_service(struct netservice **n)
{
    delete_timer(&(*n)->timer);
    CloseEngine((*n)->engine);
    free(*n);
    *n = NULL;
}
