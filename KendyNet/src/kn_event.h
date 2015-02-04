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
	int ret = 0;
	if(h->events == 0){
		if(h->type == KN_SOCKET)
			events |= EPOLLRDHUP;
		ret = kn_event_add(e,h,events);
	}else
		ret = kn_event_mod(e,h,events);
		
	if(ret == 0)
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
	int ret = 0;
	if(h->events == 0){
		if(h->type == KN_SOCKET)
			events |= EPOLLRDHUP;
		ret = kn_event_add(e,h,events);
	}else
		ret = kn_event_mod(e,h,events);
		
	if(ret == 0)
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
	int ret;
	if(h->events == 0){
		ret = kn_event_add(e,h,EVFILT_READ);
	}else
		ret = kn_event_enable(e,h,EVFILT_READ);
		
	if(ret == 0)
		h->events |= EVFILT_READ | 0x80000000;
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
	int ret;
	if(h->events == 0){
		ret = kn_event_add(e,h,EVFILT_WRITE);
	}else
		ret = kn_event_enable(e,h,EVFILT_WRITE);
		
	if(ret == 0)
		h->events |= EVFILT_WRITE | 0x80000000;
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
