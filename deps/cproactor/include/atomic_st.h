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


#define DECLARE_ATOMIC_TYPE(NAME,TYPE)\
struct NAME##_st{\
	volatile int32_t version;\
	TYPE data;\
};\
struct NAME\
{\
	uint32_t g_version;\
	int32_t index;\
	volatile struct NAME##_st *ptr;\
	struct NAME##_st array[2];\
};\
static inline TYPE NAME##_get(struct NAME *at)\
{\
	TYPE ret;\
	while(1)\
	{\
		struct NAME##_st *ptr_p = (struct NAME##_st *)at->ptr;\
		int save_version = ptr_p->version;\
		_FENCE;\
		ret = ptr_p->data;\
        _FENCE;\
		if(ptr_p == at->ptr && save_version == ptr_p->version)\
			break;\
		ATOMIC_INCREASE(&miss_count);\
	}\
	ATOMIC_INCREASE(&get_count);\
	return ret;\
}\
static inline void NAME##_set(struct NAME *at,TYPE v)\
{\
	at->array[at->index].data = v;\
	at->array[at->index].version = ++at->g_version;\
    _FENCE;\
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
	free(at);\
}
