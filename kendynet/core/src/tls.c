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
    struct tls_st *all_tls = (struct tls_st*)ud;
    uint16_t i = 0;
    for(; i < MAX_TLS_SIZE;++i)
    {
        if(all_tls[i].destroy_fn)
        {
            all_tls[i].destroy_fn(all_tls[i].tls_val);
        }
    }
    free(ud);
}

static void tls_once_routine(){
    pthread_key_create(&g_tls_key,tls_destroy_fn);
}


int32_t tls_create(uint16_t key,void *ud,TLS_DESTROY_FN fn)
{
    if(key >= MAX_TLS_SIZE || !ud) return -1;
    pthread_once(&g_tls_key_once,tls_once_routine);
    struct tls_st *all_tls = (struct tls_st *)pthread_getspecific(g_tls_key);
    if(!all_tls){
        all_tls = calloc(1,sizeof(struct tls_st*));
        pthread_setspecific(g_tls_key,(void*)all_tls);
    }
    all_tls[key].tls_val = ud;
    all_tls[key].destroy_fn = fn;
    return 0;
}


void* tls_get(uint32_t key)
{
    if(key >= MAX_TLS_SIZE) return NULL;
    pthread_once(&g_tls_key_once,tls_once_routine);
    struct tls_st *all_tls = (struct tls_st *)pthread_getspecific(g_tls_key);
    if(!all_tls){
        all_tls = calloc(1,sizeof(struct tls_st*));
        pthread_setspecific(g_tls_key,(void*)all_tls);
    }
    return all_tls[key].tls_val;
}









