#include "refbase.h"
#include "systime.h"

static atomic_16_t g_counter[65536] = {0};

void ref_init(struct refbase *r,uint16_t type,void (*destroyer)(void*),int32_t initcount)
{
	r->destroyer = destroyer;
	r->high32 = GetSystemMs();
	uint32_t _type = type;
	r->low32  = _type*65536 + (uint32_t)(ATOMIC_INCREASE(&g_counter[type]));
	r->refcount = initcount;
}
