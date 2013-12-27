#include "refbase.h"
#include "SysTime.h"

atomic_32_t global_counter = 0;

void ref_init(struct refbase *r,void (*destroyer)(void*),int32_t initcount)
{
	r->destroyer = destroyer;
	r->identity = ATOMIC_INCREASE(&global_counter);
	r->identity <<= 32;
	r->identity += GetSystemMs();
	r->refcount = initcount;
}