#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "buffer.h"

allocator_t buffer_allocator = NULL;

static void buffer_destroy(void *b)
{
	buffer_t _b = (buffer_t)b;
	if(_b->next)
		buffer_release(&(_b)->next);
    FREE(buffer_allocator,_b);
	b = 0;
}

static inline buffer_t buffer_create(uint32_t capacity)
{
	uint32_t size = sizeof(struct buffer) + capacity;
    buffer_t b = (buffer_t)ALLOC(buffer_allocator,size);
	if(b)
	{
		b->size = 0;
		b->capacity = capacity;
		ref_init(&b->_refbase,0,buffer_destroy,0);
	}
	return b;
}


buffer_t buffer_create_and_acquire(buffer_t b,uint32_t capacity)
{
	buffer_t nb = buffer_create(capacity);
	return buffer_acquire(b,nb);
}
