/*	
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "atomic.h"
volatile int get_count;
volatile int set_count;
volatile int miss_count;

struct atomic_st
{
	volatile int32_t version;
	char data[];
};

struct atomic_type
{
	uint32_t g_version;
	int32_t index;
	volatile struct atomic_st *ptr;
	int32_t data_size;
	struct atomic_st* array[2];	
};

struct atomic_type *create_atomic_type(uint32_t size);
void destroy_atomic_type(struct atomic_type **_at);

#define GET_ATOMIC_ST(NAME,TYPE)\
static inline void NAME(struct atomic_type *at,TYPE *ret)\
{\
	while(1)\
	{\
		struct atomic_st *ptr_p = (struct atomic_st *)at->ptr;\
		int save_version = ptr_p->version;\
		int s=at->data_size;\
		int i = 0;\
		if(at->data_size%4==0)\
			for(;s>0;++i,s-=4)((int32_t*)ret->base.data)[i]=((int32_t*)ptr_p->data)[i];\
		else if(at->data_size%2==0)\
			for(;s>0;++i,s-=2)((int16_t*)ret->base.data)[i]=((int16_t*)ptr_p->data)[i];\
		else\
			memcpy(ret->base.data,ptr_p->data,at->data_size);\
        _FENCE;\
		if(ptr_p == at->ptr && save_version == ptr_p->version)\
			break;\
		ATOMIC_INCREASE(&miss_count);\
	}\
	ATOMIC_INCREASE(&get_count);\
}

#define SET_ATOMIC_ST(NAME,TYPE)\
static inline void NAME(struct atomic_type *at,TYPE *p)\
{\
	struct atomic_st *new_p = at->array[at->index];\
	at->index ^= 0x1;\
	int s=at->data_size;\
	int i = 0;\
	if(at->data_size%4==0)\
		for(;s>0;++i,s-=4)((int32_t*)new_p->data)[i]=((int32_t*)p->base.data)[i];\
	else if(at->data_size%2==0)\
		for(;s>0;++i,s-=2)((int16_t*)new_p->data)[i]=((int16_t*)p->base.data)[i];\
	else\
		for(;i<s;++i)new_p->data[i]=p->base.data[i];\
    _FENCE;\
	new_p->version = ++at->g_version;\
    _FENCE;\
	at->ptr = new_p;\
	ATOMIC_INCREASE(&set_count);\
}
