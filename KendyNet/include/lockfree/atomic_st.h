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
#include "kn_atomic.h"
#include "kn_common_define.h"    
volatile int get_count;
volatile int set_count;
volatile int miss_count;


#define DECLARE_ATOMIC_TYPE(NAME,TYPE)\
struct NAME##_st{\
	volatile int32_t version;\
	TYPE data;\
};\
struct NAME\
{\
	uint32_t g_version;\
	int32_t index;\
	struct NAME##_st * volatile ptr;\
	struct NAME##_st array[2];\
};\
static inline void NAME##_get(struct NAME *at,TYPE *ret)\
{\
	if(unlikely(!at || !ret)) return;\
	while(1){\
		struct NAME##_st *ptr_p = (struct NAME##_st *)at->ptr;\
		int save_version = ptr_p->version;\
		FENCE();\
		*ret = ptr_p->data;\
		if(ptr_p == at->ptr){\
			FENCE();\
			if(save_version == ptr_p->version)\
				break;\
		}\
		ATOMIC_INCREASE(&miss_count);\
	}\
	ATOMIC_INCREASE(&get_count);\
}\
static inline void NAME##_set(struct NAME *at,TYPE *v)\
{\
	if(unlikely(!at || !v)) return;\
	at->array[at->index].data = *v;\
	FENCE();\
	at->array[at->index].version = ++at->g_version;\
	FENCE();\
	at->ptr = &at->array[at->index];\
	at->index ^= 0x1;\
	ATOMIC_INCREASE(&set_count);\
}\
static inline struct NAME *NAME##_new()\
{\
	struct NAME *at = calloc(1,sizeof(*at));\
	at->index = 0;\
	at->g_version = 0;\
	at->ptr = NULL;\
	return at;\
}\
static inline void NAME##_free(struct NAME *at)\
{\
	if(at) free(at);\
}
