#include "kn_ref.h"
#include "kn_time.h"

atomic_32_t g_ref_counter = 0;

void kn_ref_init(kn_ref *r,void (*destroyer)(void*))
{
	r->destroyer = destroyer;
	r->high32 = kn_systemms();
	r->low32  = (uint32_t)(ATOMIC_INCREASE(&g_ref_counter));
	r->refcount = 1;
}
