/*	
    Copyright (C) <2012>  <huangweilook@21cn.com>

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
#ifndef _THREAD_H
#define _THREAD_H
#include <pthread.h>
#include <stdint.h>
struct thread;
typedef struct thread *thread_t;

typedef void *(*thread_routine)(void*);

enum
{
	THREAD_DETACH = 0,
	THREAD_JOINABLE = 1,
};

thread_t create_thread(int32_t);
void destroy_thread(thread_t*);
void*thread_join(thread_t);
void thread_start_run(thread_t,thread_routine,void*);

#ifndef CREATE_THREAD_RUN
#define CREATE_THREAD_RUN(JOINABLE,ROUTINE,ARG)\
({thread_t __t;__t =create_thread(JOINABLE);\
  thread_start_run(__t,ROUTINE,ARG);__t;})
#endif

//直接开启一个线程,运行thread_routine
void  thread_run(thread_routine,void*);
void thread_suspend(thread_t,int32_t);
void thread_resume(thread_t);


#endif
