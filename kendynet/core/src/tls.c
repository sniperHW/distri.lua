#include "tls.h"
#include <stdlib.h>


struct tls_st
{
    void *tls_val;
    TLS_DESTROY_FN destroy_fn;
};

static pthread_key_t g_tls_key;
static pthread_once_t g_tls_key_once = PTHREAD_ONCE_INIT;

static void tls_destroy_fn(void *ud)
{
    struct tls_st *thd_tls = (struct tls_st*)ud;
    uint16_t i = 0;
    for(; i < MAX_TLS_SIZE;++i)
        if(thd_tls[i].tls_val && thd_tls[i].destroy_fn)
            thd_tls[i].destroy_fn(thd_tls[i].tls_val);
    free(ud);
}

static void tls_once_routine(){
    pthread_key_create(&g_tls_key,tls_destroy_fn);
}


int32_t tls_create(uint16_t key,void *ud,TLS_DESTROY_FN fn)
{
    if(key >= MAX_TLS_SIZE || !ud) return -1;
    pthread_once(&g_tls_key_once,tls_once_routine);
    struct tls_st *thd_tls = (struct tls_st *)pthread_getspecific(g_tls_key);
    if(!thd_tls){
        thd_tls = calloc(1,sizeof(struct tls_st*));
        pthread_setspecific(g_tls_key,(void*)thd_tls);
    }else if(thd_tls[key].tls_val || thd_tls[key].destroy_fn)
		return -1;
    thd_tls[key].tls_val = ud;
    thd_tls[key].destroy_fn = fn;
    return 0;
}


void* tls_get(uint32_t key)
{
    if(key >= MAX_TLS_SIZE) return NULL;
    pthread_once(&g_tls_key_once,tls_once_routine);
    struct tls_st *thd_tls = (struct tls_st *)pthread_getspecific(g_tls_key);
    if(!thd_tls) return NULL;
    return thd_tls[key].tls_val;
}

int32_t  tls_set(uint32_t key,void *ud)
{
    if(key >= MAX_TLS_SIZE) return -1;
    pthread_once(&g_tls_key_once,tls_once_routine);
    struct tls_st *thd_tls = (struct tls_st *)pthread_getspecific(g_tls_key);
    if(!thd_tls) return -1;
    if(thd_tls[key].tls_val && thd_tls[key].destroy_fn)
		thd_tls[key].destroy_fn(thd_tls[key].tls_val);
	thd_tls[key].tls_val = ud;
	return 0;	
}










