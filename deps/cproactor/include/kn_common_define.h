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
	CHANNEL        = 5,
	REDISCONN      = 6,
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

//消息类型定义
enum
{
	//逻辑到通信
	MSG_USERMSG = 0,
	MSG_WPACKET,
	MSG_RPACKET,
};


#define MAX_UINT32 0xffffffff
#define likely(x) __builtin_expect(!!(x), 1)  
#define unlikely(x) __builtin_expect(!!(x), 0)

static inline int32_t is_pow2(uint32_t size)
{
	return !(size&(size-1));
}

static inline uint32_t size_of_pow2(uint32_t size)
{
    if(is_pow2(size)) return size;
	size = size-1;
	size = size | (size>>1);
	size = size | (size>>2);
	size = size | (size>>4);
	size = size | (size>>8);
	size = size | (size>>16);
	return size + 1;
}

static inline uint8_t get_pow2(uint32_t size)
{
	uint8_t pow2 = 0;
    if(!is_pow2(size)) size = (size << 1);
	while(size > 1){
		pow2++;
		size = size >> 1;
	}
	return pow2;
}

#ifdef USE_MUTEX
#define LOCK_TYPE kn_mutex_t
#define LOCK(L) kn_mutex_lock(L)
#define UNLOCK(L) kn_mutex_unlock(L)
#define LOCK_CREATE kn_mutex_create
#define LOCK_DESTROY(L) kn_mutex_destroy(L) 
#else
#define LOCK_TYPE spinlock_t
#define LOCK(L) spin_lock(L)
#define UNLOCK(L) spin_unlock(L)
#define LOCK_CREATE spin_create 
#define LOCK_DESTROY(L) spin_destroy(L) 
#endif	

#endif
