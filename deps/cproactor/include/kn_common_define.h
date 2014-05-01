#ifndef _COMMON_DEFINE_H
#define _COMMON_DEFINE_H

#include <stdint.h>
#include "kn_list.h"
#include "kn_sockaddr.h"

enum{
	SCLOSE = 0,
	SESTABLISH = 1 << 1,  //双向通信正常
	SWCLOSE    = 1 << 2,     //写关闭
	SRCLOSE    = 1 << 3,     //读关闭
};

enum{
	STREAM_SOCKET  = 1,
	DGRAM_SOCKET   = 2,
	ACCEPTOR       = 3,
	CONNECTOR      = 4,
};

typedef struct
{
    kn_list_node      next;
    kn_sockaddr       addr;//供udp使用
	struct            iovec *iovec;
	int32_t           iovec_count;
}st_io;

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression)\
	({ long int __result;\
	do __result = (long int)(expression);\
	while(__result == -1L&& errno == EINTR);\
	__result;})
#endif

#endif
