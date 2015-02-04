#ifdef _LINUX

#include "kn_epoll.h"

#elif _FREEBSD

#include "kn_kqueue.h"

#else

#error "un support platform!"		

#endif