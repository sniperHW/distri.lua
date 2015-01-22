#ifndef _OBJPOOL_H
#define _OBJPOOL_H

#include "kn_alloc.h"

allocator_t objpool_new(size_t objsize,uint32_t initnum);
void objpool_destroy(allocator_t);

#endif