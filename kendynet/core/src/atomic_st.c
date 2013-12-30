#include "atomic_st.h"
#include <stdlib.h>
#include <stdio.h>
volatile int get_count = 0;
volatile int set_count = 0;
volatile int miss_count = 0;
struct atomic_type *create_atomic_type(uint32_t size)
{
	struct atomic_type *at = calloc(1,sizeof(*at));
	at->index = 0;
	at->g_version = 0;
	at->ptr = NULL;
	at->array[0] = calloc(1,size);
	at->array[1] = calloc(1,size);
	at->data_size = size - sizeof(struct atomic_st);
	return at;
}

void destroy_atomic_type(struct atomic_type **_at)
{
	struct atomic_type *at = *_at;
	free(at->array[0]);
	free(at->array[1]);
	free(at);
	_at = NULL;
}
