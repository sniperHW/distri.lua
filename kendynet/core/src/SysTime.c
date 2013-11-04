#include "SysTime.h"

pthread_key_t g_systime_key;
pthread_once_t g_systime_key_once = PTHREAD_ONCE_INIT;
volatile uint32_t g_global_ms = 0;
volatile time_t g_global_sec = 0;


void* systick_routine(void *arg){
	while(1){
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        g_global_ms =ts.tv_sec * 1000 + ts.tv_nsec/1000000;
		g_global_sec = time(NULL);
		sleepms(1);
	}
	return NULL;
}
