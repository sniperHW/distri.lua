#include "kendynet.h"
#include "kn_socket.h"
#include "kn_timer.h"
#include "kn_timerfd.h"
#include <assert.h>

struct st_notify{
	struct st_head comm_head;
	int fd_write;
};

typedef struct{
	int epfd;
	struct epoll_event* events;
	int    eventsize;
	int    maxevents;
	handle_t timerfd;	
	struct st_notify notify_stop;
}kn_epoll;

int kn_event_add(engine_t e,handle_t h,int events){
	assert((events & EPOLLET) == 0);
	int ret;
	struct epoll_event ev = {0};
	kn_epoll *ep = (kn_epoll*)e;
	struct st_head *s = (struct st_head*)h;
	ev.data.ptr = s;
	ev.events = events;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(ep->epfd,EPOLL_CTL_ADD,s->fd,&ev));
	if(ret != 0) return errno;
	++ep->eventsize;
	if(ep->eventsize > ep->maxevents){
		free(ep->events);
		ep->maxevents <<= 2;
		ep->events = calloc(1,sizeof(*ep->events)*ep->maxevents);
	}
	return 0;
}

int kn_event_mod(engine_t e,handle_t h,int events){
	assert((events & EPOLLET) == 0);	
	int ret;
	struct epoll_event ev = {0};
	kn_epoll *ep = (kn_epoll*)e;
	struct st_head *s = (struct st_head*)h;
	ev.data.ptr = s;
	ev.events = events;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(ep->epfd,EPOLL_CTL_MOD,s->fd,&ev));
	return ret;	
}

int kn_event_del(engine_t e,handle_t h){
	kn_epoll *ep = (kn_epoll*)e;
	struct st_head *s = (struct st_head*)h;	
	struct epoll_event ev = {0};
	int ret;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(ep->epfd,EPOLL_CTL_DEL,s->fd,&ev));
	if(0 == ret){ 
		--ep->eventsize;
	}
	return ret;	
}

engine_t kn_new_engine(){
	int epfd = epoll_create(1);
	if(epfd < 0) return NULL;
	int tmp[2];
	if(pipe(tmp) != 0){
		close(epfd);
		return NULL;
	}		
	kn_epoll *ep = calloc(1,sizeof(*ep));
	ep->epfd = epfd;
	ep->maxevents = 1024;
	ep->events = calloc(1,(sizeof(*ep->events)*ep->maxevents));
	ep->notify_stop.comm_head.fd = tmp[0];
	ep->notify_stop.fd_write = tmp[1];
	fcntl(tmp[0], F_SETFL, O_NONBLOCK | O_RDWR);
	fcntl(tmp[1], F_SETFL, O_NONBLOCK | O_RDWR);
	kn_event_add(ep,&ep->notify_stop,EPOLLIN);
	
	ep->timerfd = kn_new_timerfd(1000);
	((struct st_head*)ep->timerfd)->ud = kn_new_timermgr();
	kn_event_add(ep,ep->timerfd,EPOLLIN | EPOLLOUT);	
	return (engine_t)ep;
}

void kn_release_engine(engine_t e){
	kn_epoll *ep = (kn_epoll*)e;
	close(ep->epfd);
	close(ep->notify_stop.comm_head.fd);
	close(ep->notify_stop.fd_write);
	free(ep->events);
	free(ep);
}

int kn_engine_run(engine_t e){
	kn_epoll *ep = (kn_epoll*)e;
	for(;;){
		errno = 0;
		int i;
		struct st_head *s;
		int nfds = TEMP_FAILURE_RETRY(epoll_wait(ep->epfd,ep->events,ep->maxevents,1000));
		if(nfds > 0){
			for(i=0; i < nfds ; ++i)
			{
				s = (struct st_head*)ep->events[i].data.ptr;
				if(s){ 
					if(s == (struct st_head*)&ep->notify_stop){
						for(;;){
							char buf[4096];
							int ret = TEMP_FAILURE_RETRY(read(ep->notify_stop.comm_head.fd,buf,4096));
							if(ret <= 0) break;
						}
						return 0;
					}else
						s->on_events(s,ep->events[i].events);
				}
			}
		}
	}
}

void kn_stop_engine(engine_t e){
	kn_epoll *ep = (kn_epoll*)e;
	TEMP_FAILURE_RETRY(write(ep->notify_stop.fd_write,(void*)1,1));
}


kn_timer_t _kn_reg_timer(kn_timermgr_t t,uint64_t timeout,kn_cb_timer cb,void *ud);

kn_timer_t kn_reg_timer(engine_t e,uint64_t timeout,kn_cb_timer cb,void *ud){
	kn_epoll *ep = (kn_epoll*)e;
	return _kn_reg_timer(((struct st_head*)ep->timerfd)->ud,timeout,cb,ud);
}
