#include "KendyNet.h"
#include "Engine.h"
#include "Socket.h"
#include "link_list.h"
#include "sock_util.h"
#include <assert.h>
#include "double_link.h"
#include "SysTime.h"

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

static inline engine_t GetEngineByHandle(ENGINE handle)
{
	return (engine_t)handle;
}

static inline void  ReleaseEngine(ENGINE handle)
{
	engine_t e = (engine_t)handle;
	free_engine(&e);
}


int32_t EngineRun(ENGINE engine,int32_t timeout)
{
	engine_t e = GetEngineByHandle(engine);
	if(!e)
		return -1;
	return e->Loop(e,timeout);
}

ENGINE CreateEngine()
{
	ENGINE engine = create_engine();
	if(engine != INVALID_ENGINE)
	{
		engine_t e = GetEngineByHandle(engine);
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

int32_t Bind2Engine(ENGINE e,SOCK s,OnIoFinish _iofinish,
        OnClearPending _clear_pending)
{
    //iofinish should not be NULL
    if(!_iofinish)
        return -1;

	engine_t engine = GetEngineByHandle(e);
	socket_t sock   = get_socket_wrapper(s);
    if(!engine || ! sock || sock->engine)
		return -1;

	if(setNonblock(s) != 0)
        return -1;
	sock->io_finish = _iofinish;
	sock->clear_pending_io = _clear_pending;
	sock->engine = engine;
	sock->socket_type = DATA;
	if(engine->Register(engine,sock) == 0)
		return 0;
	else
		sock->engine = NULL;
	return -1;
}

SOCK EListen(ENGINE e,const char *ip,int32_t port,void*ud,OnAccept accept_function)
{
	engine_t engine = GetEngineByHandle(e);
	if(!engine) return INVALID_SOCK;
	SOCK ListenSocket;
	ListenSocket = Tcp_Listen(ip,port,256);
	if(ListenSocket == INVALID_SOCK) return INVALID_SOCK;
	if(setNonblock(ListenSocket) != 0)
    {
       CloseSocket(ListenSocket);
       return INVALID_SOCK;
    }
	socket_t sock   = get_socket_wrapper(ListenSocket);
	sock->on_accept = accept_function;
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
	engine_t engine = GetEngineByHandle(e);
	if(!engine) return -1;
	return engine->WakeUp(engine);
}

int32_t EConnect(ENGINE e,const char *ip,int32_t port,void *ud,OnConnect _on_connect,uint32_t ms)
{
    engine_t engine = GetEngineByHandle(e);
	if(!engine) return -1;
    if(ip == NULL || _on_connect == 0)
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
        _on_connect(sock,&servaddr,ud,0);
    else{
        if(errno != EINPROGRESS)
        {
            CloseSocket(sock);
            return -1;
        }
        s->socket_type = CONNECT;
        s->timeout = GetSystemMs() + ms;
        s->sock = sock;
        s->engine = engine;
        s->ud = ud;
        s->on_connect = _on_connect;
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
	LINK_LIST_PUSH_BACK(&s->pending_recv,io);
	if(s->engine && s->readable)
	{
		double_link_push(&s->engine->actived,(struct double_link_node*)s);
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
	LINK_LIST_PUSH_BACK(&s->pending_send,io);
	if(s->engine && s->writeable)
	{
		double_link_push(&s->engine->actived,(struct double_link_node*)s);
	}
	return 0;
}
