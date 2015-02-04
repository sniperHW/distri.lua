#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <unistd.h>
#include "kendynet_private.h"
#include "kn_timer.h"
#include "kn_timer_private.h"
#include "kn_timerfd.h"
#include "kn_event.h"
#include <assert.h>

struct st_notify{
	handle comm_head;
	int fd_write;
};

typedef struct{
	int kfd;
	struct kevent* events;
	int    maxevents;
	handle_t timerfd;	
	struct st_notify notify_stop;
}kn_kqueue;

int kn_event_add(engine_t e,handle_t h,int events){
	struct kevent ke;
	kn_kqueue *kq = (kn_kqueue*)e;
	EV_SET(&ke, h->fd, EVFILT_READ, EV_ADD, 0, 0, h);
	return kevent(kq->kfd, &ke, 1, NULL, 0, NULL);	
}

int kn_event_del(engine_t e,handle_t h){
	struct kevent ke;
	kn_kqueue *kq = (kn_kqueue*)e;
	EV_SET(&ke, h->fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	kevent(kq->kfd, &ke, 1, NULL, 0, NULL);
	EV_SET(&ke, h->fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
	kevent(kq->kfd, &ke, 1, NULL, 0, NULL);
	h->events = 0;	
	return 0;	
}

int kn_event_enable(engine_t e,handle_t h,int event){
	struct kevent ke;
	kn_kqueue *kq = (kn_kqueue*)e;
	EV_SET(&ke, h->fd, event,EV_ENABLE, 0, 0, h);
	return kevent(kq->kfd, &ke, 1, NULL, 0, NULL);
}

int kn_event_disable(engine_t e,handle_t h,int event){
	struct kevent ke;
	kn_kqueue *kq = (kn_kqueue*)e;
	EV_SET(&ke, h->fd, event,EV_DISABLE, 0, 0, h);
	return kevent(kq->kfd, &ke, 1, NULL, 0, NULL);
}

engine_t kn_new_engine(){
	int kfd = kqueue();
	if(kfd < 0) return NULL;
	int tmp[2];
	if(pipe2(tmp,O_NONBLOCK|O_CLOEXEC) != 0){
		close(kfd);
		return NULL;
	}		
	kn_kqueue *kq = calloc(1,sizeof(*kq));
	kq->kfd = kfd;
	kq->maxevents = 64;
	kq->events = calloc(1,(sizeof(*kq->events)*kq->maxevents));
	kq->notify_stop.comm_head.fd = tmp[0];
	kq->notify_stop.fd_write = tmp[1];
	kn_event_add(kq,(handle_t)&kq->notify_stop,EVFILT_READ);
	return (engine_t)kq;
}

void kn_release_engine(engine_t e){
	kn_kqueue *kq = (kn_kqueue*)e;
	close(kq->kfd);
	close(kq->notify_stop.comm_head.fd);
	close(kq->notify_stop.fd_write);
	free(kq->events);
	free(kq);
}

void kn_engine_runonce(engine_t e,uint32_t ms){
	kn_kqueue *kq = (kn_kqueue*)e;
	errno = 0;
	int i;
	handle_t h;
	struct timespec ts;
	uint64_t msec;
	clock_gettime(CLOCK_UPTIME, &ts);
	msec = ms%1000;
	ts.tv_nsec += (msec*1000*1000);
	ts.tv_sec  += (ms/1000);
	if(ts.tv_nsec >= 1000*1000*1000){
		ts.tv_sec += 1;
		ts.tv_nsec %= (1000*1000*1000);
	}	
	int nfds = TEMP_FAILURE_RETRY(kevent(kq->kfd, NULL, 0, kq->events,kq->maxevents, &ts));
	if(nfds > 0){
		for(i=0; i < nfds ; ++i)
		{
			h = (handle_t)kq->events[i].udata;
			if(h){ 
				if(h == (handle_t)&kq->notify_stop){
					for(;;){
						char buf[4096];
						int ret = TEMP_FAILURE_RETRY(read(h->fd,buf,4096));
						if(ret <= 0) break;
					}
					return;
				}else
					h->on_events(h,kq->events[i].filter);
			}
		}
		if(nfds == kq->maxevents){
			printf("kqueue expand\n");
			free(kq->events);
			kq->maxevents <<= 2;
			kq->events = calloc(1,sizeof(*kq->events)*kq->maxevents);
		}				
	}	
}

int kn_engine_run(engine_t e){
	kn_kqueue *kq = (kn_kqueue*)e;
	for(;;){
		errno = 0;
		int i;
		handle_t h;
		int nfds = TEMP_FAILURE_RETRY(kevent(kq->kfd, NULL, 0, kq->events,kq->maxevents, NULL));
		if(nfds > 0){
			for(i=0; i < nfds ; ++i)
			{
				h = (handle_t)kq->events[i].udata;
				if(h){ 
					if(h == (handle_t)&kq->notify_stop){
						for(;;){
							char buf[4096];
							int ret = TEMP_FAILURE_RETRY(read(h->fd,buf,4096));
							if(ret <= 0) break;
						}
						return 0;
					}else
						h->on_events(h,kq->events[i].filter);
				}
			}
			if(nfds == kq->maxevents){
				printf("kqueue expand\n");
				free(kq->events);
				kq->maxevents <<= 2;
				kq->events = calloc(1,sizeof(*kq->events)*kq->maxevents);
			}				
		}	
	}
}

void kn_stop_engine(engine_t e){
	kn_kqueue *kq = (kn_kqueue*)e;
	char buf[1];
	TEMP_FAILURE_RETRY(write(kq->notify_stop.fd_write,buf,1));
}


kn_timer_t kn_reg_timer(engine_t e,uint64_t timeout,kn_cb_timer cb,void *ud){
	/*kn_kqueue *kq = (kn_kqueue*)e;
	if(!kq->timerfd){
		kq->timerfd = kn_new_timerfd(1);
		((handle_t)kq->timerfd)->ud = kn_new_timermgr();
		struct kevent change;
  		struct kevent event;
   		EV_SET(&change, 1, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, 1, 0);		
	}
	return reg_timer_imp(((handle_t)kq->timerfd)->ud,timeout,cb,ud);
	*/
	return NULL;
}
