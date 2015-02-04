#ifdef _LINUX

#include "kn_epoll.h"

#elif _BSD

#include "kn_kqueue.h"

#else

#error "un support platform!"		

#endif