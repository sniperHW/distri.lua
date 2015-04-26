#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <unistd.h>
#include "kendynet_private.h"
#include "kn_timer.h"
#include "kn_timer_private.h"
#include "kn_timerfd.h"
#include "kn_event.h"
#include <assert.h>
#include "kn_list.h"

struct st_notify{
	handle comm_head;
	int fd_write;
};

typedef struct{
	int epfd;
	struct epoll_event* events;
	int    maxevents;
	handle_t timerfd;	
	struct st_notify notify_stop;
	//kn_list wait_destroy;
}kn_epoll;

int kn_event_add(engine_t e,handle_t h,int events){
	assert((events & EPOLLET) == 0);
	int ret;
	struct epoll_event ev = {0};
	kn_epoll *ep = (kn_epoll*)e;
	ev.data.ptr = h;
	ev.events = events;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(ep->epfd,EPOLL_CTL_ADD,h->fd,&ev));
	if(ret != 0) return errno;
	h->events = events;
	return 0;
}

int kn_event_mod(engine_t e,handle_t h,int events){
	assert((events & EPOLLET) == 0);	
	int ret;
	struct epoll_event ev = {0};
	kn_epoll *ep = (kn_epoll*)e;
	ev.data.ptr = h;
	ev.events = events;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(ep->epfd,EPOLL_CTL_MOD,h->fd,&ev));
	if(0 == ret) h->events = events;		
	return ret;	
}

int kn_event_del(engine_t e,handle_t h){
	kn_epoll *ep = (kn_epoll*)e;
	struct epoll_event ev = {0};
	int ret;
	errno = 0;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(ep->epfd,EPOLL_CTL_DEL,h->fd,&ev));
	if(0 == ret) h->events = 0;
	return ret;	
}

/*void   kn_push_destroy(engine_t e,handle_t h){
	kn_epoll *ep = (kn_epoll*)e;
	kn_list_pushback(&ep->wait_destroy,(kn_list_node*)h);
}

static inline void kn_process_destroy(engine_t e){
	kn_epoll *ep = (kn_epoll*)e;
	kn_list_node *n;
	while((n = kn_list_pop(&ep->wait_destroy))){
		((handle_t)n)->on_destroy(n);
	}
}*/

engine_t kn_new_engine(){
	int epfd = epoll_create1(EPOLL_CLOEXEC);
	if(epfd < 0) return NULL;
	int tmp[2];
	if(pipe2(tmp,O_NONBLOCK|O_CLOEXEC) != 0){
		close(epfd);
		return NULL;
	}		
	kn_epoll *ep = calloc(1,sizeof(*ep));
	ep->epfd = epfd;
	ep->maxevents = 64;
	ep->events = calloc(1,(sizeof(*ep->events)*ep->maxevents));
	ep->notify_stop.comm_head.fd = tmp[0];
	ep->notify_stop.fd_write = tmp[1];
	kn_event_add(ep,(handle_t)&ep->notify_stop,EPOLLIN);
	return (engine_t)ep;
}

void kn_release_engine(engine_t e){
	kn_epoll *ep = (kn_epoll*)e;
	if(ep->timerfd)
		kn_timerfd_destroy(ep->timerfd);
	close(ep->epfd);
	close(ep->notify_stop.comm_head.fd);
	close(ep->notify_stop.fd_write);
	free(ep->events);
	free(ep);
}

void kn_engine_runonce(engine_t e,uint32_t ms,uint32_t max_process_time){
	kn_epoll *ep = (kn_epoll*)e;
	errno = 0;
	int i;
	handle_t h;
	if(max_process_time < ms) 
		max_process_time = ms; 
	uint64_t timeout = kn_systemms64() + max_process_time;
	int nfds = TEMP_FAILURE_RETRY(epoll_wait(ep->epfd,ep->events,ep->maxevents,ms));
	if(nfds > 0){
		for(i=0; i < nfds && kn_systemms64() < timeout; ++i)
		{
			h = (handle_t)ep->events[i].data.ptr;
			if(h){ 
				if(h == (handle_t)&ep->notify_stop){
					for(;;){
						char buf[32];
						int ret = TEMP_FAILURE_RETRY(read(h->fd,buf,32));
						if(ret <= 0) break;
					}
					return;
				}else
					h->on_events(h,ep->events[i].events);
			}
		}
		//kn_process_destroy(e);
		if(nfds == ep->maxevents){
			free(ep->events);
			ep->maxevents <<= 2;
			ep->events = calloc(1,sizeof(*ep->events)*ep->maxevents);
		}			
	}else if(nfds < 0){
		abort();
	}
		
}

int kn_engine_run(engine_t e){
	kn_epoll *ep = (kn_epoll*)e;
	for(;;){
		errno = 0;
		int i;
		handle_t h;
		int nfds = TEMP_FAILURE_RETRY(epoll_wait(ep->epfd,ep->events,ep->maxevents,-1));
		if(nfds > 0){
			for(i=0; i < nfds ; ++i)
			{
				h = (handle_t)ep->events[i].data.ptr;
				if(h){ 
					if(h == (handle_t)&ep->notify_stop){
						for(;;){
							char buf[32];
							int ret = TEMP_FAILURE_RETRY(read(h->fd,buf,32));
							if(ret <= 0) break;
						}
						return 0;
					}else
						h->on_events(h,ep->events[i].events);
				}
			}
			//kn_process_destroy(e);
			if(nfds == ep->maxevents){
				free(ep->events);
				ep->maxevents <<= 2;
				ep->events = calloc(1,sizeof(*ep->events)*ep->maxevents);
			}				
		}else if(nfds < 0){
			abort();
		}	
	}
}

void kn_stop_engine(engine_t e){
	kn_epoll *ep = (kn_epoll*)e;
	char buf[1];
	TEMP_FAILURE_RETRY(write(ep->notify_stop.fd_write,buf,1));
}


kn_timer_t kn_reg_timer(engine_t e,uint64_t timeout,int32_t(*cb)(uint32_t,void*),void *ud){
	kn_epoll *ep = (kn_epoll*)e;
	if(!ep->timerfd){
		ep->timerfd = kn_new_timerfd(1);
		((handle_t)ep->timerfd)->ud = wheelmgr_new();
		kn_event_add(ep,ep->timerfd,EPOLLIN | EPOLLOUT);			
	}
	return wheelmgr_register(((handle_t)ep->timerfd)->ud,timeout,cb,ud);
}

int is_set_read(handle *h){
	return h->events & EPOLLIN;
}

int is_set_write(handle *h){
	return h->events & EPOLLOUT;
}