#include "kn_time.h"

pthread_key_t g_systime_key;

#ifdef __GNUC__
pthread_once_t g_systime_key_once = PTHREAD_ONCE_INIT;

__attribute__((constructor(101))) static void systick_init(){
    pthread_key_create(&g_systime_key,NULL);
}

#endif
