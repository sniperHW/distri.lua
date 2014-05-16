#include "kn_epoll.h"
#include "kn_time.h"
#include "kn_fd.h"
#include "kn_connector.h"
#include "kn_datasocket.h"
#include "kn_dlist.h"
#include "redisconn.h"

extern  int kn_max_proactor_fd; 

int32_t  kn_epoll_register(kn_proactor_t p,kn_fd_t s){
	int ret;
	int type = s->type;
	struct epoll_event ev = {0};
	kn_epoll *ep = (kn_epoll*)p;
	if(s->proactor) return -1;
	if(ep->eventsize >= kn_max_proactor_fd) return -1;
	ev.data.ptr = s;
	if(type == STREAM_SOCKET){
		ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
	}else if(type == DGRAM_SOCKET){
		ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
	}else if(type == ACCEPTOR){
		ev.events = EPOLLIN;
	}else if(type == CONNECTOR){
		ev.events = EPOLLIN | EPOLLOUT;
	}else if(type == CHANNEL){
		ev.events = EPOLLIN;
	}else if(type == REDISCONN){
		redisconn_t rc = (redisconn_t)s;
		if(rc->state == REDIS_CONNECTING){
			ev.events = EPOLLIN | EPOLLOUT;
		}else{
			ev.events = EPOLLIN | EPOLLRDHUP;
		}
	}else
		return -1;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(ep->epfd,EPOLL_CTL_ADD,s->fd,&ev));
	if(ret != 0) return -errno;
	s->events = ev.events;
	++ep->eventsize;

	if(ep->eventsize > ep->maxevents){
		free(ep->events);
		ep->maxevents <<= 2;
		ep->events = calloc(1,sizeof(*ep->events)*ep->maxevents);
	}

	if(type == CONNECTOR){
		kn_dlist_push(&p->connecting,(kn_dlist_node*)s);
	}
	s->proactor = p;
	return 0;
}

int32_t  kn_epoll_unregister(kn_proactor_t p ,kn_fd_t s){
	kn_epoll *ep = (kn_epoll*)p;
	struct epoll_event ev = {0};
	int32_t ret;
	if(!s->proactor) return -1;	
	TEMP_FAILURE_RETRY(ret = epoll_ctl(ep->epfd,EPOLL_CTL_DEL,s->fd,&ev));
	//((kn_datasocket*)s)->flag = 0;
    kn_dlist_remove((kn_dlist_node*)s);
	s->proactor = NULL;
	if(0 == ret){ 
		--ep->eventsize;
		s->events = 0;
	}
	return ret;
}

//以下4个函数只为兼容redis异步接口使用
static int32_t addRead(kn_proactor_t p ,kn_fd_t s)
{
	if(s->type != REDISCONN) return -1;
	int ret;
	struct epoll_event ev = {0};
	kn_epoll *ep = (kn_epoll*)p;
	if(ep->eventsize >= kn_max_proactor_fd) return -1;
	ev.data.ptr = s;
	ev.events = s->events | EPOLLIN | EPOLLRDHUP;
	int op = EPOLL_CTL_ADD;
	if(s->proactor) op = EPOLL_CTL_MOD; 
	TEMP_FAILURE_RETRY(ret = epoll_ctl(ep->epfd,op,s->fd,&ev));
	if(ret != 0) return -errno;
	s->events = ev.events;
	if(!s->proactor){
		++ep->eventsize;
		if(ep->eventsize > ep->maxevents){
			free(ep->events);
			ep->maxevents <<= 2;
			ep->events = calloc(1,sizeof(*ep->events)*ep->maxevents);
		}
		s->proactor = p;	
	}
	return 0;	
}

static int32_t delRead(kn_proactor_t p ,kn_fd_t s)
{
	if(s->type != REDISCONN) return -1;
	if(!s->proactor) return -1;
	struct epoll_event ev = {0};
	kn_epoll *ep = (kn_epoll*)p;	
	ev.data.ptr = s;
	ev.events = s->events & (~EPOLLIN);	
	s->events = ev.events;	
	return epoll_ctl(ep->epfd,EPOLL_CTL_MOD,s->fd,&ev);
}

static int32_t addWrite(kn_proactor_t p ,kn_fd_t s)
{
	if(s->type != REDISCONN) return -1;
	int ret;
	struct epoll_event ev = {0};
	kn_epoll *ep = (kn_epoll*)p;
	if(ep->eventsize >= kn_max_proactor_fd) return -1;
	ev.data.ptr = s;
	ev.events = s->events | EPOLLOUT;
	int op = EPOLL_CTL_ADD;
	if(s->proactor) op = EPOLL_CTL_MOD; 		
	TEMP_FAILURE_RETRY(ret = epoll_ctl(ep->epfd,op,s->fd,&ev));
	if(ret != 0) return -errno;
	s->events = ev.events;
	if(!s->proactor){
		++ep->eventsize;
		if(ep->eventsize > ep->maxevents){
			free(ep->events);
			ep->maxevents <<= 2;
			ep->events = calloc(1,sizeof(*ep->events)*ep->maxevents);
		}
		s->proactor = p;	
	}
	return 0;		
}

static int32_t delWrite(kn_proactor_t p ,kn_fd_t s)
{
	if(s->type != REDISCONN) return -1;
	if(!s->proactor) return -1;
	if(!s->proactor) return -1;
	struct epoll_event ev = {0};
	kn_epoll *ep = (kn_epoll*)p;	
	ev.data.ptr = s;
	ev.events = s->events & (~EPOLLOUT);
	s->events = ev.events;	
	return epoll_ctl(ep->epfd,EPOLL_CTL_MOD,s->fd,&ev);	
}



static int32_t _epoll_wait(int epfd, struct epoll_event *events,int maxevents, int timeout)
{
	uint64_t _timeout = kn_systemms64() + (uint64_t)timeout;
	if(0 == maxevents)
		usleep(timeout*1000);
	else{
		for(;;){
			int32_t nfds = epoll_wait(epfd,events,maxevents,timeout);
			if(nfds < 0 && errno == EINTR){
				uint64_t cur_tick = kn_systemms64();
				if(_timeout > cur_tick){
					timeout = (int)(_timeout - cur_tick);
					errno = 0;
				}
				else
					return 0;
			}else
				return nfds;
		}	
	}
	return 0;
}

static int8_t check_connect_timeout(kn_dlist_node *dln, void *ud)
{
	//printf("check_connect_timeout\n");
	kn_connector_t c = (kn_connector_t)dln;
	uint64_t l_now = *((uint64_t*)ud);
    if(l_now >= c->timeout){
        //c->base.proactor->UnRegister(c->base.proactor,(kn_fd_t)c);
        c->cb_connected(NULL,&c->remote,c->base.ud,ETIMEDOUT);
        kn_closefd((kn_fd_t)c);
        return 1;
    }
    return 0;
}

int32_t kn_epoll_loop(kn_proactor_t p,int32_t ms)
{
	uint64_t    sleep_ms;
	uint64_t    timeout = kn_systemms64() + (uint64_t)ms;
	uint64_t    current_tick;
	int32_t     nfds;
	int32_t     i;
	uint64_t    l_now;
	kn_dlist*   actived;
	kn_fd_t     s;
	kn_epoll*   ep = (kn_epoll*)p;
	if(ms < 0) ms = 0;
	do{
		if(!kn_dlist_empty(&p->connecting)){
			l_now = kn_systemms64();
            kn_dlist_check_remove(&p->connecting,check_connect_timeout,(void*)&l_now);
		}
		actived = kn_proactor_activelist(p);	
        if(!kn_dlist_empty(actived)){
            p->actived_index = (p->actived_index+1)%2;
            while((s = (kn_fd_t)kn_dlist_pop(actived)) != NULL){
                if(s->process(s))
                    kn_procator_putin_active(p,s);
            }
        }
       	current_tick = kn_systemms64();
       	actived = kn_proactor_activelist(p);
        if(kn_dlist_empty(actived))
			sleep_ms = timeout > current_tick ? timeout - current_tick:0;
		else
			sleep_ms = 0;
        nfds = _epoll_wait(ep->epfd,ep->events,ep->eventsize,(uint32_t)sleep_ms);
		if(nfds < 0)
			return -1;
		
		for(i=0; i < nfds ; ++i)
		{
			s = (kn_fd_t)ep->events[i].data.ptr;
			s->on_active(s,ep->events[i].events);
		}
		current_tick = kn_systemms64();
	}while(timeout > current_tick);
	
	if(!kn_dlist_empty(&p->service)){
		struct service *cur = (struct service*)kn_dlist_first(&p->service);
		struct service *last = (struct service*)kn_dlist_last(&p->service);
		do{
			if(cur->tick) cur->tick(cur);
			if(cur == last) break;
			cur = (struct service*)((kn_dlist_node*)cur)->next;
		}while(1); 
	}	
	return 0;
}


void *testaddr;

kn_epoll* kn_epoll_new()
{
	int epfd = epoll_create(kn_max_proactor_fd);
	if(epfd < 0) return NULL;	
	kn_epoll *ep = calloc(1,sizeof(*ep));
	ep->epfd = epfd;
	ep->maxevents = 1024;
	ep->events = calloc(1,(sizeof(*ep->events)*ep->maxevents));
	kn_dlist_init(&ep->base.actived[0]);
	kn_dlist_init(&ep->base.actived[1]);
	kn_dlist_init(&ep->base.service);
	ep->base.actived_index = 0;
	kn_dlist_init(&ep->base.connecting);
	ep->base.Loop = kn_epoll_loop;
	ep->base.Register = kn_epoll_register;
	ep->base.UnRegister = kn_epoll_unregister;
	ep->base.addRead = addRead;
	ep->base.delRead = delRead;
	ep->base.addWrite = addWrite;
	ep->base.delWrite = delWrite;	
	testaddr = (void*)ep;
	return ep;
}

void kn_epoll_del(kn_epoll *ep)
{
	
}
