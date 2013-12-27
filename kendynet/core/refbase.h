#ifndef _REFBASE_H
#define _REFBASE_H

#include <stdint.h>
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
	atomic_32_t new_count = ATOMIC_DECREASE(&r->refcount);
    if(new_count <= 0 && r->destroyer){
		r->identity = 0;
		_FENCE;
		while(!COMPARE_AND_SWAP(&r->flag,0,1));
		r->destroyer(r);
	}
	return new_count;
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
	ident _ident = {0,0};
	return _ident;
}

static inline struct refbase *cast_2_refbase(ident _ident)
{
	while(_ident.identity == _ident.ptr->identity)
	{
		if(COMPARE_AND_SWAP(&_ident.ptr->flag,0,1))
		{
			if(_ident.identity == _ident.ptr->identity){
				ref_increase(_ident.ptr);
				_FENCE;
				_ident.ptr->flag = 0;
				return _ident.ptr;
			}else
				return 0;
		}
	}
	return 0;
}

static inline int32_t is_ident_vaild(ident _ident)
{
	if(!_ident.ptr || !_ident.identity) return 0;
	return 1;
}
#endif
