#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "buffer.h"

kn_allocator_t buffer_allocator = NULL;

uint32_t buffer_count = 0;

/*
buffer_t buffer_create_and_acquire(buffer_t b,uint32_t capacity)
{
	buffer_t nb = buffer_create(capacity);
	return buffer_acquire(b,nb);
}*/
