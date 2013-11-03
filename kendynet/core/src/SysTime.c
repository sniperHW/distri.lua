#include "SysTime.h"

pthread_key_t g_systime_key;
pthread_once_t g_systime_key_once = PTHREAD_ONCE_INIT;
volatile uint32_t g_global_ms = 0;
volatile uint32_t g_global_sec = 0;


void* systick_routine(void *arg){
	while(1){
		struct timeval now;
		gettimeofday(&now, NULL);
		g_global_ms = now.tv_sec*1000+now.tv_usec/1000;
		g_global_sec = time(NULL);
		sleepms(1);
	}
	return NULL;
}
