#include "kendynet.h"
#include "poller.h"
#include "socket.h"
#include "llist.h"
#include "sock_util.h"
#include <assert.h>
#include "dlist.h"
#include "systime.h"

void (*destroy_stio)(st_io*) = NULL;

socket_t get_socket_wrapper(SOCK s);
void init_socket_pool();
void destroy_socket_pool();
SOCK OpenSocket(int32_t family,int32_t type,int32_t protocol);

int32_t InitNetSystem()
{
	signal(SIGPIPE,SIG_IGN);
	return 0;
}


void  CleanNetSystem(){}

static inline poller_t GetEngineByHandle(ENGINE handle)
{
    return (poller_t)handle;
}

static inline void  ReleaseEngine(ENGINE handle)
{
    poller_t e = (poller_t)handle;
    poller_delete(&e);
}


int32_t EngineRun(ENGINE engine,int32_t timeout)
{
    poller_t e = GetEngineByHandle(engine);
	if(!e)
		return -1;
	return e->Loop(e,timeout);
}

ENGINE CreateEngine()
{
    ENGINE engine = poller_new();
	if(engine != INVALID_ENGINE)
	{
        poller_t e = GetEngineByHandle(engine);
		if(0 != e->Init(e))
		{
			CloseEngine(engine);
			engine = INVALID_ENGINE;
		}
	}
	return engine;
}

void CloseEngine(ENGINE handle)
{
	ReleaseEngine(handle);
}

int32_t Bind2Engine(ENGINE e,SOCK s,CB_IOFINISH iofinish)
{
    //iofinish should not be NULL
    if(!iofinish)
        return -1;

    poller_t engine = GetEngineByHandle(e);
	socket_t sock   = get_socket_wrapper(s);
    if(!engine || ! sock || sock->engine)
		return -1;

	if(setNonblock(s) != 0)
        return -1;
    sock->io_finish = iofinish;
	sock->engine = engine;
	sock->socket_type = DATA;
	if(engine->Register(engine,sock) == 0)
		return 0;
	else
		sock->engine = NULL;
	return -1;
}

SOCK EListen(ENGINE e,const char *ip,int32_t port,void*ud,CB_ACCEPT on_accept)
{
    poller_t engine = GetEngineByHandle(e);
    if(!engine || ! on_accept) return INVALID_SOCK;
	SOCK ListenSocket;
	ListenSocket = Tcp_Listen(ip,port,256);
	if(ListenSocket == INVALID_SOCK) return INVALID_SOCK;
	if(setNonblock(ListenSocket) != 0)
    {
       CloseSocket(ListenSocket);
       return INVALID_SOCK;
    }
	socket_t sock   = get_socket_wrapper(ListenSocket);
    sock->on_accept = on_accept;
	sock->ud = ud;
	sock->engine = engine;
	sock->socket_type = LISTEN;
	if(engine->Register(engine,sock) == 0)
		return ListenSocket;
	else{
		CloseSocket(ListenSocket);
	}
	return INVALID_SOCK;
}

int32_t EWakeUp(ENGINE e)
{
    poller_t engine = GetEngineByHandle(e);
	if(!engine) return -1;
	return engine->WakeUp(engine);
}

int32_t EConnect(ENGINE e,const char *ip,int32_t port,void *ud,CB_CONNECT on_connect,uint32_t ms)
{
    poller_t engine = GetEngineByHandle(e);
	if(!engine) return -1;
    if(ip == NULL || on_connect == 0)
        return -1;

    struct sockaddr_in servaddr;
	memset((void*)&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = INET;
    servaddr.sin_port = htons(port);
    if(inet_pton(INET,ip,&servaddr.sin_addr) < 0)
	{
		printf("%s\n",strerror(errno));
		return -1;
	}
	SOCK sock = OpenSocket(INET,STREAM,TCP);
	if(sock == INVALID_SOCK)
        return -1;
    if(setNonblock(sock) != 0){
        CloseSocket(sock);
        return -1;
    }
    socket_t s = get_socket_wrapper(sock);
    if(connect(s->fd,(const struct sockaddr *)&servaddr,sizeof(servaddr)) == 0)
        on_connect(sock,&servaddr,ud,0);
    else{
        if(errno != EINPROGRESS)
        {
            CloseSocket(sock);
            return -1;
        }
        s->socket_type = CONNECT;
        s->timeout = GetSystemMs64() + (uint64_t)ms;
        s->sock = sock;
        s->engine = engine;
        s->ud = ud;
        s->on_connect = on_connect;
        s->addr_remote = servaddr;
        if(engine->Register(engine,s) == 0)
            return 0;
        else{
            CloseSocket(sock);
            return -1;
        }
    }
	return 0;
}

int32_t Recv(SOCK sock,st_io *io,uint32_t *err_code)
{
	assert(io);
	socket_t s = get_socket_wrapper(sock);
    if(!s || !test_recvable(s->status))
	{
		*err_code = 0;
		return -1;
	}
	int32_t ret = raw_recv(s,io,err_code);
	if(ret == 0) return -1;
	if(ret < 0 && *err_code == EAGAIN)return 0;
	return ret;
}

int32_t Post_Recv(SOCK sock,st_io *io)
{
	assert(io);
	socket_t s = get_socket_wrapper(sock);
    if(!s || !test_recvable(s->status))
		return -1;
    LLIST_PUSH_BACK(&s->pending_recv,io);
	if(s->engine && s->readable)
	{
        putin_active(s->engine,(struct dnode*)s);
	}
	return 0;
}

int32_t Send(SOCK sock,st_io *io,uint32_t *err_code)
{
	assert(io);
	socket_t s = get_socket_wrapper(sock);
    if(!s || !test_sendable(s->status))
	{
		*err_code = 0;
		return -1;
	}
	int32_t ret = raw_send(s,io,err_code);
	if(ret == 0) return -1;
	if(ret < 0 && *err_code == EAGAIN)return 0;
	return ret;
}

int32_t Post_Send(SOCK sock,st_io *io)
{
	assert(io);
	socket_t s = get_socket_wrapper(sock);
    if(!s || !test_sendable(s->status))
		return -1;
    LLIST_PUSH_BACK(&s->pending_send,io);
	if(s->engine && s->writeable)
	{
        putin_active(s->engine,(struct dnode*)s);
	}
	return 0;
}
