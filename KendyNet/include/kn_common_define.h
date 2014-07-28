#ifndef _COMMON_DEFINE_H
#define _COMMON_DEFINE_H

#include <stdint.h>
#include "kn_list.h"


typedef struct
{
    kn_list_node      next;
    void*             ud;
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


#define MAX_UINT32 0xffffffff
#define likely(x) __builtin_expect(!!(x), 1)  
#define unlikely(x) __builtin_expect(!!(x), 0)


#endif
