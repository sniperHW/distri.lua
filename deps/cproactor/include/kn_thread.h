/*
    Copyright (C) <2014>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/	
#ifndef _KN_THREAD_H
#define _KN_THREAD_H
#include <pthread.h>
#include <stdint.h>
#include "kn_thread_sync.h"


typedef void *(*kn_thread_routine)(void*);

typedef struct thread
{
	pthread_t threadid;
	volatile int32_t is_suspend;
	int32_t joinable;
	kn_mutex_t mtx;
	kn_condition_t  cond;
}*kn_thread_t;


enum
{
	THREAD_DETACH = 0,
	THREAD_JOINABLE = 1,
};

kn_thread_t kn_create_thread(int32_t);
void        kn_thread_destroy(kn_thread_t);
void*       kn_thread_join(kn_thread_t);
void        kn_thread_start_run(kn_thread_t,kn_thread_routine,void*);
static inline pthread_t   kn_thread_getid(kn_thread_t t){
	return t->threadid;
}

#ifndef CREATE_THREAD_RUN
#define CREATE_THREAD_RUN(JOINABLE,ROUTINE,ARG)\
({kn_thread_t __t;__t =create_thread(JOINABLE);\
  kn_thread_start_run(__t,ROUTINE,ARG);__t;})
#endif

//直接开启一个线程,运行thread_routine
void        kn_thread_run(kn_thread_routine,void*);
void        kn_thread_suspend(kn_thread_t,int32_t);
void        kn_thread_resume(kn_thread_t);


#endif
