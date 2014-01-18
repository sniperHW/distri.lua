#ifndef _BUFFER_H
#define _BUFFER_H
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
/*
* 带引用计数的buffer
*/

#include <stdint.h>
#include <string.h>
#include "refbase.h"
#include "llist.h"
#include "allocator.h"

typedef struct buffer
{
	struct refbase _refbase;
	uint32_t capacity;
	uint32_t size;
	struct buffer *next;
	int8_t   buf[0];
}*buffer_t;


extern allocator_t buffer_allocator;
buffer_t buffer_create_and_acquire(buffer_t,uint32_t);

static inline void buffer_release(buffer_t *b)
{
	if(*b){
		ref_decrease(&(*b)->_refbase);
        *b = NULL;
	}
}

static inline buffer_t buffer_acquire(buffer_t b1,buffer_t b2)
{
    if(b1 == b2) return b1;
    if(b2) ref_increase(&b2->_refbase);
    if(b1) buffer_release(&b1);
	return b2;
}

/*
* 从b的pos开始读取size字节长的数据,如果长度大于b->size-pos,会尝试从b->next中读出剩余部分
*/

static inline int buffer_read(buffer_t b,uint32_t pos,int8_t *out,uint32_t size)
{
	uint32_t copy_size;
	while(size){
        if(!b) return -1;
        if(pos >= b->size) return -1;
        copy_size = b->size - pos;
		copy_size = copy_size > size ? size : copy_size;
		memcpy(out,b->buf + pos,copy_size);
		size -= copy_size;
		pos += copy_size;
		out += copy_size;
		if(pos >= b->size){
			pos = 0;
			b = b->next;
		}
	}
	return 0;
}


/*
*将长度为size字节的数据写入到b的pos开始位置，如果size大于b->capacity-pos,会尝试将剩余部分写入到
*b->next中
*/
static inline int buffer_write(buffer_t b,uint32_t pos,int8_t *in,uint32_t size)
{
    uint32_t copy_size;
    while(size){
        if(!b) return -1;
        if(pos >= b->capacity) return -1;
        copy_size = b->capacity - pos;
        copy_size = copy_size > size ? size : copy_size;
        memcpy(b->buf + pos,in,copy_size);
        size -= copy_size;
        pos += copy_size;
        in += copy_size;
        if(pos >= b->capacity){
            pos = 0;
            b = b->next;
        }
    }
    return 0;
}

static inline int32_t is_pow2(uint32_t size)
{
	return !(size&(size-1));
}

static inline uint32_t size_of_pow2(uint32_t size)
{
    if(is_pow2(size)) return size;
	size = size-1;
	size = size | (size>>1);
	size = size | (size>>2);
	size = size | (size>>4);
	size = size | (size>>8);
	size = size | (size>>16);
	return size + 1;
}

static inline uint8_t get_pow2(uint32_t size)
{
	uint8_t pow2 = 0;
    if(!is_pow2(size)) size = (size << 1);
	while(size > 1){
		pow2++;
		size = size >> 1;
	}
	return pow2;
}

#endif
