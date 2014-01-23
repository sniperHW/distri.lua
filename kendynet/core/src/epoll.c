#include "epoll.h"
#include "socket.h"
#include "sock_util.h"
#include "systime.h"
#include "common.h"
#include <assert.h>


int32_t  epoll_init(poller_t e)
{
	assert(e);
	e->poller_fd = TEMP_FAILURE_RETRY(epoll_create(MAX_SOCKET));
	if(e->poller_fd < 0) return -1;
	memset(e->events,0,sizeof(e->events));
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
    ev.events = EV_IN;
	if(0 != epoll_ctl(e->poller_fd,EPOLL_CTL_ADD,e->pipe_reader,&ev))
	{
		close(e->pipe_reader);
		close(e->pipe_writer);
		close(e->poller_fd);
		return -1;
	}
	return 	0;
}

int32_t epoll_register(poller_t e, socket_t s)
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
        dlist_push(&e->connecting,(struct dnode*)s);
    }else
        return -1;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(e->poller_fd,EPOLL_CTL_ADD,s->fd,&ev));
	return ret;
}


int32_t epoll_unregister(poller_t e,socket_t s)
{
	assert(e);assert(s);
	struct epoll_event ev;int32_t ret;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(e->poller_fd,EPOLL_CTL_DEL,s->fd,&ev));
	s->readable = s->writeable = 0;
    dlist_remove((struct dnode*)s);
	s->engine = NULL;
	return ret;
}

int32_t  epoll_unregister_recv(poller_t e,socket_t s)
{
    assert(e);assert(s);
    struct epoll_event ev;int32_t ret;
    if(s->socket_type == DATA){
		ev.data.ptr = s;
        ev.events = EV_OUT | EV_ET | EPOLLRDHUP;
        TEMP_FAILURE_RETRY(ret = epoll_ctl(e->poller_fd,EPOLL_CTL_MOD,s->fd,&ev));
    }
    else if(s->socket_type == LISTEN)
        TEMP_FAILURE_RETRY(ret = epoll_ctl(e->poller_fd,EPOLL_CTL_DEL,s->fd,&ev));
    else
        return -1;
    s->readable = 0;
    if(s->writeable == 0 || LLIST_IS_EMPTY(&s->pending_send))
        dlist_remove((struct dnode*)s);
    return ret;
}

int32_t  epoll_unregister_send(poller_t e,socket_t s)
{
    assert(e);assert(s);
    struct epoll_event ev;int32_t ret;
    if(s->socket_type == DATA){
		ev.data.ptr = s;
        ev.events = EV_IN | EV_ET | EPOLLRDHUP;
        TEMP_FAILURE_RETRY(ret = epoll_ctl(e->poller_fd,EPOLL_CTL_MOD,s->fd,&ev));
    }else
        return -1;
    s->writeable = 0;
    if(s->readable == 0 || LLIST_IS_EMPTY(&s->pending_recv))
        dlist_remove((struct dnode*)s);
    return ret;
}

int8_t check_connect_timeout(struct dnode *dln, void *ud)
{
    socket_t s = (socket_t)dln;
    uint64_t l_now = *((uint64_t*)ud);
    if(l_now >= s->timeout){
        s->engine->UnRegister(s->engine,s);
        s->on_connect(INVALID_SOCK,&s->addr_remote,s->ud,ETIMEDOUT);
        CloseSocket(s->sock);
        return 1;
    }
    return 0;
}

int32_t epoll_wakeup(poller_t e)
{
	return write(e->pipe_writer,"x",1);
}

static int32_t _epoll_wait(int epfd, struct epoll_event *events,int maxevents, int timeout)
{
	uint64_t _timeout = GetSystemMs64() + (uint64_t)timeout;
	for(;;){
        int32_t nfds = epoll_wait(epfd,events,MAX_SOCKET+1,timeout);
		if(nfds < 0 && errno == EINTR){
			uint64_t cur_tick = GetSystemMs64();
			if(_timeout > cur_tick){
				timeout = (int)(_timeout - cur_tick);
				errno = 0;
			}
			else
				return 0;
		}else
			return nfds;
	}	
	return 0;
}

int32_t epoll_loop(poller_t n,int32_t ms)
{
	assert(n);
	if(ms < 0)ms = 0;
	uint64_t sleep_ms;
	uint64_t timeout = GetSystemMs64() + (uint64_t)ms;
	uint64_t current_tick;
	uint32_t read_event = EV_IN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
	int32_t notify = 0;
	do{

        if(!dlist_empty(&n->connecting))
	    {
	        //check timeout connecting
	        uint64_t l_now = GetSystemMs64();
            dlist_check_remove(&n->connecting,check_connect_timeout,(void*)&l_now);
	    }
        if(!is_active_empty(n))
        {
            struct dlist *actived = get_active_list(n);
            n->actived_index = (n->actived_index+1)%2;
            socket_t s;
            while((s = (socket_t)dlist_pop(actived)) != NULL)
            {
                if(Process(s))
                    putin_active(n,(struct dnode*)s);
            }
        }
		current_tick = GetSystemMs64();
        if(is_active_empty(n))
			sleep_ms = timeout > current_tick ? timeout - current_tick:0;
		else
			sleep_ms = 0;
		notify = 0;
        int32_t nfds = _epoll_wait(n->poller_fd,n->events,MAX_SOCKET,(uint32_t)sleep_ms);
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
		current_tick = GetSystemMs64();
	}while(notify == 0 && timeout > current_tick);
	return 0;
}
