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
#ifndef _SYNC_H
#define _SYNC_H
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
/*Mutex*/
typedef struct mutex
{
	pthread_mutex_t     m_mutex;
	pthread_mutexattr_t m_attr;
}*mutex_t;

mutex_t mutex_create();
void mutex_destroy(mutex_t *m);

static inline int32_t mutex_lock(mutex_t m)
{
	return pthread_mutex_lock(&m->m_mutex);
}

static inline int32_t mutex_try_lock(mutex_t m)
{
	return pthread_mutex_trylock(&m->m_mutex);
}

static inline int32_t mutex_unlock(mutex_t m)
{
	return pthread_mutex_unlock(&m->m_mutex);
}

/*Condition*/
typedef struct condition
{
	pthread_cond_t cond;
}*condition_t;


condition_t condition_create();
void condition_destroy(condition_t *c);

static inline int32_t condition_wait(condition_t c,mutex_t m)
{
	return pthread_cond_wait(&c->cond,&m->m_mutex);
}



static int32_t condition_timedwait(condition_t c,mutex_t m,int32_t ms)
{
	struct timespec ts;
	//clock_gettime(1/*CLOCK_REALTIME*/, &ts);
	//ts.tv_sec += 1;
	struct timeval now;
	gettimeofday(&now, NULL);
	uint64_t sec = ms/1000;
	uint64_t msec = ms%1000;
	ts.tv_sec = now.tv_sec + sec;
	ts.tv_nsec = now.tv_usec * 1000 + msec*1000*1000;   

	return pthread_cond_timedwait(&c->cond,&m->m_mutex,&ts);
}

static inline int32_t condition_signal(condition_t c)
{
	return pthread_cond_signal(&c->cond);
}

static inline int32_t condition_broadcast(condition_t c)
{
	return pthread_cond_broadcast(&c->cond);
}

/*Barrior*/
typedef struct barrior
{
	condition_t cond;
	mutex_t 	mtx;
	int32_t     wait_count;
}*barrior_t;

barrior_t barrior_create(int32_t);
void barrior_destroy(barrior_t*);

static inline void barrior_wait(barrior_t b)
{
	mutex_lock(b->mtx);
	--b->wait_count;
	if(0 == b->wait_count)
	{
		condition_broadcast(b->cond);
	}else	
	{
		while(b->wait_count > 0)
		{
			condition_wait(b->cond,b->mtx);
		}
	}
	mutex_unlock(b->mtx);
}

#endif
