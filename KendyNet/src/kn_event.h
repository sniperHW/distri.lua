#ifndef _KN_EVENT_H
#define _KN_EVENT_H

#include <assert.h>
#include "kn_type.h"


#ifdef _LINUX

#include    <sys/epoll.h>

enum{
	EVENT_READ  =  (EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP),
	EVENT_WRITE =  EPOLLOUT,	
};

static inline int      kn_enable_read(engine_t e,handle *h){
	int events = h->events | EPOLLIN;		
	if(kn_event_mod(e,h,events) == 0)
		h->events = events;
	else{
		assert(0);
		return -1;
	}
	return 0;
}

static inline int      kn_disable_read(engine_t e,handle *h){
          return kn_event_mod(e,h,h->events & (~EPOLLIN));
}

static inline int      kn_enable_write(engine_t e,handle *h){
	int events = h->events | EPOLLOUT;		
	if(kn_event_mod(e,h,events) == 0)
		h->events = events;
	else{
		assert(0);
		return -1;
	}
	return 0;
}

static inline int      kn_disable_write(engine_t e,handle *h){
          return kn_event_mod(e,h,h->events & (~EPOLLOUT));
}

#elif _FREEBSD

#include    <sys/event.h>

enum{
	EVENT_READ  =  EVFILT_READ,
	EVENT_WRITE =  EVFILT_WRITE,	
};

static inline int      kn_enable_read(engine_t e,handle *h){		
	if(kn_event_enable(e,h,EVFILT_READ) == 0)
		h->events |= EVFILT_READ;
	else{
		assert(0);
		return -1;
	}
	return 0;	
}

static inline int      kn_disable_read(engine_t e,handle *h){
          if(0 == kn_event_disable(e,h,EVFILT_READ))
          {
          	h->events &= (~EVFILT_READ);
          	return 0;
          }
          return -1;
}

static inline int      kn_enable_write(engine_t e,handle *h){		
	if(kn_event_enable(e,h,EVFILT_WRITE) == 0)
		h->events |= EVFILT_WRITE;
	else{
		assert(0);
		return -1;
	}
	return 0;
}

static inline int      kn_disable_write(engine_t e,handle *h){
          if(0 == kn_event_disable(e,h,EVFILT_WRITE))
          {
          	h->events &= (~EVFILT_WRITE);
          	return 0;
          }
          return -1;
}

#else

#error "un support platform!"	

#endif

#endif
