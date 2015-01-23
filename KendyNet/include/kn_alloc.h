#ifndef _KN_ALLOC_H
#define _KN_ALLOC_H

#include <stdlib.h>
#include <stdint.h>

typedef struct allocator{
	void* (*alloc)(struct allocator*,size_t);
	void* (*calloc)(struct allocator*,size_t,size_t);
	void* (*realloc)(struct allocator*,void*,size_t);
	void   (*free)(struct allocator*,void*);
}allocator,*allocator_t;

#define ALLOC(A,SIZE)\
	({void* __result;\
	   if(!A) 	__result = malloc(SIZE);\
	   else  __result = ((allocator_t)A)->alloc((allocator_t)A,SIZE);\
	  __result;})

#define CALLOC(A,NUM,SIZE)\
	({void* __result;\
	   if(!A) 	__result = calloc(NUM,SIZE);\
	   else  __result = ((allocator_t)A)->calloc((allocator_t)A,NUM,SIZE);\
	  __result;})

#define REALLOC(A,PTR,SIZE) do{if(!A) realloc(PTR,SIZE);else  ((allocator_t)A)->realloc((allocator_t)A,PTR,SIZE);}while(0)

#define FREE(A,PTR) do{if(!A) free(PTR);else   ((allocator_t)A)->free((allocator_t)A,PTR);}while(0)

#endif