#include "netservice.h"

void check_timeout(TimingWheel_t t,struct WheelItem *wit,uint32_t now)
{
    struct connection *c = wheelitem2con(wit);
    struct netservice *n = (struct netservice*)wit->ud;
    acquire_conn(c);
    if(c->_recv_timeout && now > c->last_recv + c->recv_timeout)
        c->_recv_timeout(c);
    if(!c->is_closed && c->_send_timeout)
    {
        wpacket_t wpk = (wpacket_t)link_list_head(&c->send_list);
        if(wpk && now > wpk->base.tstamp + c->send_timeout)
            c->_send_timeout(c);
    }
    if(!c->is_closed) RegisterTimer(t,wit,1000);
    release_conn(c);
}

static int32_t _bind(struct netservice *n,
							struct connection *c,
							process_packet _process_packet,
							on_disconnect _on_disconnect,
							uint32_t rtimeout,
                            on_recv_timeout _recv_timeout,
                            uint32_t stimeout,
							on_send_timeout _send_timeout)
 {
    c->_recv_timeout = _recv_timeout;
    c->_send_timeout = _send_timeout;
    c->recv_timeout = rtimeout;
    c->send_timeout = stimeout;
    c->wheelitem.ud = (void*)n;
    c->wheelitem.callback = check_timeout;
    RegisterTimer(n->timer,con2wheelitem(c),1000);
    return bind2engine(n->engine,c,_process_packet,_on_disconnect);
 }



static SOCK _listen(struct netservice *n,
                           const char *ip,int32_t port,void *ud, OnAccept on_accept)
{
  return EListen(n->engine,ip,port,ud,on_accept);
}


int32_t _connect(struct netservice *n,const char *ip,int32_t port,void *ud,OnConnect on_connect,
                 uint32_t timeout)
{
    return EConnect(n->engine,ip,port,ud,on_connect,timeout);
}

int32_t _loop(struct netservice *n,uint32_t ms)
{
    UpdateWheel(n->timer,GetSystemMs());
    return EngineRun(n->engine,ms);
}

struct netservice *new_service(uint32_t precision,uint32_t max)
{
    struct netservice *n = calloc(1,sizeof(*n));
    n->timer = CreateTimingWheel(precision,max);
    n->engine = CreateEngine();
    if(n->engine == INVALID_ENGINE)
    {
        DestroyTimingWheel(&n->timer);
        free(n);
        return NULL;
    }
    n->bind = _bind;
    n->listen = _listen;
    n->loop = _loop;
    n->connect = _connect;
    return n;
}

void destroy_service(struct netservice **n)
{
    DestroyTimingWheel(&(*n)->timer);
    CloseEngine((*n)->engine);
    free(*n);
    *n = NULL;
}
