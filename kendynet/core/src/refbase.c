#include "refbase.h"
#include "systime.h"

atomic_32_t global_counter = 0;

void ref_init(struct refbase *r,uint16_t type,void (*destroyer)(void*),int32_t initcount)
{
	r->destroyer = destroyer;
	uint64_t _type = (uint64_t)type;
	_type <<= 48; 
	r->identity = ATOMIC_INCREASE(&global_counter) & 0x3FFF;//高14位留作类型编码
	r->identity <<= 32;
	r->identity += GetSystemMs();
	r->identity |= _type;  
	r->refcount = initcount;
}
