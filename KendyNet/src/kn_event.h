#ifndef _KN_EVENT_H
#define _KN_EVENT_H

#include <assert.h>
#include "kn_type.h"


#ifdef _LINUX

#include    <sys/epoll.h>

enum{
	EVENT_READ  =  (EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP),
	EVENT_WRITE =  EPOLLOUT,
	//EVENT_ERROR = (EPOLLERR | EPOLLHUP | EPOLLRDHUP),	
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
          return kn_event_mod(e,h,h->events ^ EPOLLIN);
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
          return kn_event_mod(e,h,h->events ^ EPOLLOUT);
}

#elif _FREEBSD

#include    <sys/event.h>

enum{
	EVENT_READ  =  EVFILT_READ,
	EVENT_WRITE =  EVFILT_WRITE,
	//EVENT_ERROR =  0xFFFFFFFF ^ EVFILT_READ ^ EVFILT_WRITE,	
};

#else

#error "un support platform!"	

#endif

#endif