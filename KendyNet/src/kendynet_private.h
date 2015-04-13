#ifndef _KENDYNET_PRIVATE_H
#define _KENDYNET_PRIVATE_H
#include "kendynet.h"

int      kn_event_add(engine_t,handle_t,int events);
int      kn_event_del(engine_t,handle_t);
//void   kn_push_destroy(engine_t,handle_t);

#ifdef _LINUX

int      kn_event_mod(engine_t,handle_t,int events);

#elif _BSD

int kn_event_enable(engine_t,handle_t,int events);
int kn_event_disable(engine_t,handle_t,int events);

#else

#error "un support platform!"

#endif

static inline void     kn_set_noblock(int fd,int block){
    int flag = fcntl(fd, F_GETFL, 0);
    if (block) {
        flag &= (~O_NONBLOCK);
    } else {
        flag |= O_NONBLOCK;
    }
    fcntl(fd, F_SETFL, flag);
}


#endif
