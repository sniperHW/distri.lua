#ifndef _REFBASE_H
#define _REFBASE_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "atomic.h"

extern atomic_32_t global_counter;

struct refbase
{
	atomic_32_t refcount;
	atomic_64_t identity;
	atomic_32_t flag;
	void (*destroyer)(void*);
};

void ref_init(struct refbase *r,void (*destroyer)(void*),int32_t initcount);

static inline atomic_32_t ref_increase(struct refbase *r)
{
    return ATOMIC_INCREASE(&r->refcount);
}

static inline atomic_32_t ref_decrease(struct refbase *r)
{
    atomic_32_t count;
    if((count = ATOMIC_DECREASE(&r->refcount)) == 0){
		r->identity = 0;
		_FENCE;
        int32_t c = 0;
        for(;;){
            if(COMPARE_AND_SWAP(&r->flag,0,1))
                break;
            if(c < 4000){
                ++c;
                __asm__("pause");
            }else{
                struct timespec ts = { 0, 500000 };
                nanosleep(&ts, NULL);
            }
        }
		r->destroyer(r);
	}
    return count;
}

typedef struct ident
{
	uint64_t identity;
	struct refbase *ptr;
}ident;

static inline ident make_ident(struct refbase *ptr)
{
	ident _ident = {ptr->identity,ptr};
	return _ident;
}

static inline ident make_empty_ident()
{
    ident _ident = {0,NULL};
	return _ident;
}

static inline struct refbase *cast_2_refbase(ident _ident)
{
    while(_ident.identity == _ident.ptr->identity)
	{
		if(COMPARE_AND_SWAP(&_ident.ptr->flag,0,1))
		{
            struct refbase *ptr = NULL;
            if(_ident.identity == _ident.ptr->identity &&
               ref_increase(_ident.ptr) > 0)
                    ptr = _ident.ptr;
            _FENCE;
            _ident.ptr->flag = 0;
            return ptr;
		}
	}
    return NULL;
}

static inline int32_t is_vaild_ident(ident _ident)
{
	if(!_ident.ptr || !_ident.identity) return 0;
	return 1;
}

#define TO_IDENT(OTHER_IDENT) (*(ident*)&OTHER_IDENT)

#endif
