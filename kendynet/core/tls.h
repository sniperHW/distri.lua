#ifndef _TLS_H
#define _TLS_H

#include <stdint.h>
#include <pthread.h>

#define MAX_TLS_SIZE 512


typedef void (*TLS_DESTROY_FN)(void*);

int32_t tls_create(uint16_t key,void*,TLS_DESTROY_FN);

void*    tls_get(uint32_t key);

int32_t  tls_set(uint32_t key,void*);

#endif
