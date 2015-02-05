#ifndef _KN_EVENT_H
#define _KN_EVENT_H

#include <assert.h>
#include "kn_type.h"


int is_set_read(handle*h);
int is_set_write(handle*h);

#ifdef _LINUX

#include    <sys/epoll.h>

enum{
	EVENT_READ  =  (EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP),
	EVENT_WRITE =  EPOLLOUT,	
};

static inline int      kn_enable_read(engine_t e,handle *h){	
	return kn_event_mod(e,h,h->events | EPOLLIN);
}

static inline int      kn_disable_read(engine_t e,handle *h){
          return kn_event_mod(e,h,h->events & (~EPOLLIN));
}

static inline int      kn_enable_write(engine_t e,handle *h){	
          return kn_event_mod(e,h,h->events | EPOLLOUT);
}
static inline int      kn_disable_write(engine_t e,handle *h){
          return kn_event_mod(e,h,h->events & (~EPOLLOUT));         
}

#elif _BSD

#include    <sys/event.h>

enum{
	EVENT_READ  =  EVFILT_READ,
	EVENT_WRITE =  EVFILT_WRITE,	
};

static inline int      kn_enable_read(engine_t e,handle *h){		
	return kn_event_enable(e,h,EVFILT_READ);	
}

static inline int      kn_disable_read(engine_t e,handle *h){
          return kn_event_disable(e,h,EVFILT_READ);
}

static inline int      kn_enable_write(engine_t e,handle *h){		
	return kn_event_enable(e,h,EVFILT_WRITE);
}

static inline int      kn_disable_write(engine_t e,handle *h){
          return kn_event_disable(e,h,EVFILT_WRITE);
}

#else

#error "un support platform!"	

#endif

#endif
