#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include "KendyNet.h"
#include "sock_util.h"
#include "Engine.h"
#include "Socket.h"
#include <stdio.h>
#include "sync.h"
#include "SysTime.h"

SOCK _accept(socket_t s,struct sockaddr *sa,socklen_t *salen);

void destroy_socket_wrapper(void *arg)
{
	struct socket_wrapper *sw = (struct socket_wrapper *)arg;
	sw = (struct socket_wrapper *)((char*)arg - ((char*)&sw->ref - (char*)sw));
	clear_pending_send(sw);
	clear_pending_recv(sw);
	free(sw);
	printf("destroy_socket_wrapper\n");
}

SOCK new_socket_wrapper()
{
	struct socket_wrapper *sw = calloc(1,sizeof(*sw));
	sw->status = SESTABLISH;
	ref_init(&sw->ref,destroy_socket_wrapper,1);
	return (SOCK)sw;
}

void acquire_socket_wrapper(SOCK s)
{
	struct socket_wrapper *sw = (struct socket_wrapper *)s;
	ref_increase(&sw->ref);
}

void release_socket_wrapper(SOCK s)
{
	struct socket_wrapper *sw = (struct socket_wrapper *)s;
	ref_decrease(&sw->ref);
}

struct socket_wrapper *get_socket_wrapper(SOCK s)
{
	return (struct socket_wrapper *)s;
}

int32_t get_fd(SOCK s)
{
	struct socket_wrapper *sw = (struct socket_wrapper *)s;
	return sw->fd;
}

void process_connect(socket_t s)
{
    int err = 0;
    socklen_t errlen = sizeof(err);

    if (getsockopt(s->fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) {
        s->on_connect(INVALID_SOCK,s->ud,err);
        CloseSocket(s->sock);
        return;
    }

    if(err){
        errno = err;
        s->on_connect(INVALID_SOCK,s->ud,errno);
        CloseSocket(s->sock);
        return;
    }
    //connect success
    s->engine->UnRegister(s->engine,s);
    s->on_connect(s->sock,s->ud,0);
}

void process_accept(socket_t s)
{
    SOCK client;
    struct sockaddr_in ClientAddress;
    int32_t nClientLength = sizeof(ClientAddress);
    while(1)
    {
        client = _accept(s, (struct sockaddr*)&ClientAddress, (socklen_t*)&nClientLength);
        if (client == INVALID_SOCK)
            break;
        else
            s->on_accept(client,s->ud);
    }
}

void on_read_active(socket_t s)
{
    s->readable = 1;
    if(!LINK_LIST_IS_EMPTY(&s->pending_recv)){
        double_link_push(&s->engine->actived,(struct double_link_node*)s);
    }
}

void on_write_active(socket_t s)
{
    s->writeable = 1;
    if(!LINK_LIST_IS_EMPTY(&s->pending_send)){
        double_link_push(&s->engine->actived,(struct double_link_node*)s);
    }
}


int32_t raw_recv(socket_t s,st_io *io_req,uint32_t *err_code)
{
    if(s->socket_type != DATA || s->status == 0) return 0;
	*err_code = 0;
	int32_t ret  = TEMP_FAILURE_RETRY(readv(s->fd,io_req->iovec,io_req->iovec_count));
	if(ret < 0)
	{
		*err_code = errno;
		if(*err_code == EAGAIN)
		{
			s->readable = 0;
			//将请求重新放回到队列
			LINK_LIST_PUSH_FRONT(&s->pending_recv,io_req);
		}
	}
	return ret;
}


static inline void _recv(socket_t s)
{
	assert(s);
	st_io* io_req = 0;
	uint32_t err_code = 0;
	if(s->readable)
	{
		if((io_req = LINK_LIST_POP(st_io*,&s->pending_recv))!=NULL)
		{
			int32_t bytes_transfer = raw_recv(s,io_req,&err_code);
			if(bytes_transfer == 0) bytes_transfer = -1;
			if(err_code != EAGAIN)  s->io_finish(bytes_transfer,io_req,err_code);
		}
	}
}

int32_t raw_send(socket_t s,st_io *io_req,uint32_t *err_code)
{
    if(s->socket_type != DATA)return 0;
	*err_code = 0;
	int32_t ret  = TEMP_FAILURE_RETRY(writev(s->fd,io_req->iovec,io_req->iovec_count));
	if(ret < 0)
	{
		*err_code = errno;
		if(*err_code == EAGAIN)
		{
			s->writeable = 0;
			//将请求重新放回到队列
			LINK_LIST_PUSH_FRONT(&s->pending_send,io_req);
		}
	}
	return ret;
}

static inline void _send(socket_t s)
{
	assert(s);
	st_io* io_req = 0;
	uint32_t err_code = 0;
	if(s->writeable)
	{
		if((io_req = LINK_LIST_POP(st_io*,&s->pending_send))!=NULL)
		{
			int32_t bytes_transfer = raw_send(s,io_req,&err_code);
			if(bytes_transfer == 0) bytes_transfer = -1;
			if(err_code != EAGAIN)  s->io_finish(bytes_transfer,io_req,err_code);
		}
	}
}

int32_t  Process(socket_t s)
{	
	acquire_socket_wrapper((SOCK)s);
	_recv(s);
	_send(s);
	int32_t read_active = s->readable && !LINK_LIST_IS_EMPTY(&s->pending_recv);
	int32_t write_active = s->writeable && !LINK_LIST_IS_EMPTY(&s->pending_send);
	release_socket_wrapper((SOCK)s);
	return (read_active || write_active);
}

void   shutdown_recv(socket_t s)
{
    if(s->engine) s->engine->UnRegisterRecv(s->engine,s);
	s->status = (s->status | SRCLOSE);
    shutdown(s->fd,SHUT_RD);
}

void   shutdown_send(socket_t s)
{
    if(s->engine) s->engine->UnRegisterSend(s->engine,s);
	s->status = (s->status | SWCLOSE);
}

void   clear_pending_send(socket_t sw)
{
    if((sw)->clear_pending_io)
    {
        list_node *tmp;
        while((tmp = link_list_pop(&sw->pending_send))!=NULL)
            sw->clear_pending_io((st_io*)tmp);
    }
    LINK_LIST_CLEAR(&sw->pending_send);
}

void   clear_pending_recv(socket_t sw)
{
    if((sw)->clear_pending_io)
    {
        list_node *tmp;
        while((tmp = link_list_pop(&sw->pending_recv))!=NULL)
            sw->clear_pending_io((st_io*)tmp);
    }
    LINK_LIST_CLEAR(&sw->pending_recv);
}
