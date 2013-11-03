#ifndef _REFBASE_H
#define _REFBASE_H

#include <stdint.h>
#include "atomic.h"

extern int8_t mutil_thread;

struct refbase
{
	atomic_32_t refcount;
	void (*destroyer)(void*);
};

static inline void ref_increase(struct refbase *r)
{
	if(mutil_thread)
		ATOMIC_INCREASE(&r->refcount);
	else
		++r->refcount;
}

static inline void ref_decrease(struct refbase *r)
{
	if(mutil_thread)
	{
		if(ATOMIC_DECREASE(&r->refcount) <= 0 && r->destroyer)
			r->destroyer(r);
	}
	else
	{
		if(--r->refcount <= 0 && r->destroyer)
			r->destroyer(r);
	}
}


#endif
