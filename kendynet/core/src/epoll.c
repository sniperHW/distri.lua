#include "epoll.h"
#include "Socket.h"
#include "sock_util.h"
#include "SysTime.h"
#include "common.h"
#include <assert.h>


int32_t  epoll_init(engine_t e)
{
	assert(e);
	e->poller_fd = TEMP_FAILURE_RETRY(epoll_create(MAX_SOCKET));
	if(e->poller_fd < 0) return -1;
	memset(e->events,0,sizeof(e->events));
	double_link_init(&e->actived);

	//创建唤醒管道
	int p[2];
	if(pipe(p) != 0){ 
		close(e->poller_fd);
		return -1;
	}
	e->pipe_reader = p[0];
	e->pipe_writer = p[1];

	struct epoll_event ev;
	ev.data.fd = e->pipe_reader;
	ev.events = EV_OUT;
	if(0 != epoll_ctl(e->poller_fd,EPOLL_CTL_ADD,e->pipe_reader,&ev))
	{
		close(e->pipe_reader);
		close(e->pipe_writer);
		close(e->poller_fd);
		return -1;
	}
	return 	0;
}

int32_t epoll_register(engine_t e, socket_t s)
{
	assert(e);assert(s);
	int32_t ret = -1;
	struct epoll_event ev;
	ev.data.ptr = s;
	if(s->socket_type == DATA)
		ev.events = EV_IN | EV_OUT | EV_ET | EPOLLRDHUP;
	else if(s->socket_type == LISTEN)
		ev.events = EV_IN;
    else if(s->socket_type == CONNECT){
        ev.events = EV_IN | EV_OUT | EV_ET;
        double_link_push(&e->connecting,(struct double_link_node*)s);
    }else
        return -1;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(e->poller_fd,EPOLL_CTL_ADD,s->fd,&ev));
	return ret;
}


int32_t epoll_unregister(engine_t e,socket_t s)
{
	assert(e);assert(s);
	struct epoll_event ev;int32_t ret;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(e->poller_fd,EPOLL_CTL_DEL,s->fd,&ev));
	s->readable = s->writeable = 0;
	double_link_remove((struct double_link_node*)s);
	s->engine = NULL;
	return ret;
}

int32_t  epoll_unregister_recv(engine_t e,socket_t s)
{
    assert(e);assert(s);
    struct epoll_event ev;int32_t ret;
    if(s->socket_type == DATA){
        ev.events = EV_OUT | EV_ET | EPOLLRDHUP;
        TEMP_FAILURE_RETRY(ret = epoll_ctl(e->poller_fd,EPOLL_CTL_MOD,s->fd,&ev));
    }
    else if(s->socket_type == LISTEN)
        TEMP_FAILURE_RETRY(ret = epoll_ctl(e->poller_fd,EPOLL_CTL_DEL,s->fd,&ev));
    else
        return -1;
    s->readable = 0;
    if(s->writeable == 0 || LINK_LIST_IS_EMPTY(&s->pending_send))
        double_link_remove((struct double_link_node*)s);
    return ret;
}

int32_t  epoll_unregister_send(engine_t e,socket_t s)
{
    assert(e);assert(s);
    struct epoll_event ev;int32_t ret;
    if(s->socket_type == DATA){
        ev.events = EV_IN | EV_ET | EPOLLRDHUP;
        TEMP_FAILURE_RETRY(ret = epoll_ctl(e->poller_fd,EPOLL_CTL_MOD,s->fd,&ev));
    }else
        return -1;
    s->writeable = 0;
    if(s->readable == 0 || LINK_LIST_IS_EMPTY(&s->pending_recv))
        double_link_remove((struct double_link_node*)s);
    return ret;
}

int8_t check_connect_timeout(struct double_link_node *dln, void *ud)
{
    socket_t s = (socket_t)dln;
    uint32_t l_now = (uint32_t)ud;
    if(l_now >= s->timeout){
        s->engine->UnRegister(s->engine,s);
        s->on_connect(INVALID_SOCK,&s->addr_remote,s->ud,ETIMEDOUT);
        CloseSocket(s->sock);
        return 1;
    }
    return 0;
}

int32_t epoll_wakeup(engine_t e)
{
	return write(e->pipe_writer,"x",1);
}

static int32_t _epoll_wait(int epfd, struct epoll_event *events,int maxevents, int timeout)
{
	uint32_t _timeout = GetSystemMs() + (uint32_t)timeout;
	for(;;){
        int32_t nfds = epoll_wait(epfd,events,MAX_SOCKET+1,timeout);
		if(nfds < 0 && errno == EINTR){
			uint32_t cur_tick = GetSystemMs();
			if(_timeout > cur_tick){
				timeout = _timeout - cur_tick;
				errno = 0;
			}
			else
				return 0;
		}else
			return nfds;
	}	
	return 0;
}

int32_t epoll_loop(engine_t n,int32_t ms)
{
	assert(n);
	if(ms < 0)ms = 0;
	uint32_t sleep_ms;
	uint32_t timeout = GetSystemMs() + ms;
	uint32_t current_tick;
	uint32_t read_event = EV_IN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
	int32_t notify = 0;
	do{

	    if(!double_link_empty(&n->connecting))
	    {
	        //check timeout connecting
	        uint32_t l_now = GetSystemMs();
	        double_link_check_remove(&n->connecting,check_connect_timeout,(void*)l_now);
	    }
		if(!double_link_empty(&n->actived))
		{
			uint32_t size = double_link_size(&n->actived);
			uint32_t i = 0;
			for(; i < size;++i)
			{
				socket_t s = (socket_t)double_link_pop(&n->actived);
				if(Process(s)){
					double_link_push(&n->actived,(struct double_link_node*)s);
				}
			}
		}

		current_tick = GetSystemMs();
		if(double_link_empty(&n->actived))
			sleep_ms = timeout > current_tick ? timeout - current_tick:0;
		else
			sleep_ms = 0;
		notify = 0;
        int32_t nfds = _epoll_wait(n->poller_fd,n->events,MAX_SOCKET,sleep_ms);
		if(nfds < 0)
			return -1;
		int32_t i;
		for(i = 0 ; i < nfds ; ++i)
		{
			if(n->events[i].data.fd == n->pipe_reader)
			{
				char buf[1];
				read(n->pipe_reader,buf,1);
				notify = 1;
			}else{
				socket_t sock = (socket_t)n->events[i].data.ptr;
				if(sock)
				{
					if(sock->socket_type == CONNECT){
						process_connect(sock);
					}
					else if(sock->socket_type == LISTEN){
						process_accept(sock);
					}
					else{
						if(n->events[i].events & read_event)
							on_read_active(sock);
						if(n->events[i].events & EPOLLOUT)
							on_write_active(sock);
					}
				}
			}
		}
		current_tick = GetSystemMs();
	}while(notify == 0 && timeout > current_tick);
	return 0;
}
