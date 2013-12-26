#ifndef _SYSTIME_H
#define _SYSTIME_H
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include "common.h"
#include "thread.h"

extern pthread_key_t g_systime_key;
extern pthread_once_t g_systime_key_once;

extern volatile uint32_t g_global_ms;
extern volatile time_t g_global_sec;

void* systick_routine(void*);

static void systick_once_routine(){
    pthread_key_create(&g_systime_key,NULL);
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    g_global_ms =ts.tv_sec * 1000 + ts.tv_nsec/1000000;
    g_global_sec = time(NULL);
	//创建一个线程以固定的频率更新g_global_ms,此线程不会退出，知道进程结束
	thread_run(systick_routine,NULL);
}

static inline uint32_t GetSystemMs()
{
#ifdef _WIN
	return GetTickCount();
#else
	pthread_once(&g_systime_key_once,systick_once_routine);
	return g_global_ms;
#endif
}

static inline time_t GetSystemSec()
{
#ifdef _WIN
	return time(NULL);
#else
	pthread_once(&g_systime_key_once,systick_once_routine);
	return g_global_sec;
#endif
}

void   block_sigusr1();
void   unblock_sigusr1();

static inline void sleepms(uint32_t ms)
{
	block_sigusr1();
	usleep(ms*1000);
	unblock_sigusr1();
}

static inline char *GetCurrentTimeStr(char *buf)
{
	time_t _now = time(NULL);
#ifdef _WIN
	struct tm *_tm;
	_tm = localtime(&_now);
	snprintf(buf,22,"[%04d-%02d-%02d %02d:%02d:%02d]",_tm->tm_year+1900,_tm->tm_mon+1,_tm->tm_mday,_tm->tm_hour,_tm->tm_min,_tm->tm_sec);
#else
	struct tm _tm;
	localtime_r(&_now, &_tm);
	snprintf(buf,22,"[%04d-%02d-%02d %02d:%02d:%02d]",_tm.tm_year+1900,_tm.tm_mon+1,_tm.tm_mday,_tm.tm_hour,_tm.tm_min,_tm.tm_sec);
#endif
	return buf;
}
#endif
