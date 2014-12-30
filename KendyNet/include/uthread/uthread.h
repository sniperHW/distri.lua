/*	
    Copyright (C) <2012-2014>  <huangweilook@21cn.com>

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
#ifndef _UTHREAD_H
#define _UTHREAD_H
#include <stdint.h>
#include "kn_refobj.h"

typedef ident uthread_t;

/*
*    init uthread system for current thread 
*/
int                 uscheduler_init(uint32_t stack_size);

int                 uscheduler_clear();
int                 uschedule();
int                 ut_yield();
int                 ut_sleep(uint32_t ms);
int                 ut_block(uint32_t ms);
int                 ut_wakeup(uthread_t);
uthread_t     ut_spawn(void(*fun)(void*),void *param);

static inline int is_vaild_uthread(uthread_t u){
    return is_empty_ident(u);
}

/*
*   get current running uthread or current thread
*   if not running in a uthread context,the return value should be empty_ident,
*   use is_vaild_uthread() to check the return value
*/
uthread_t     ut_getcurrent();

void *ut_dic_get(uthread_t,const char *key);
int      ut_dic_set(uthread_t,const char *key,void *value,void (*value_destroyer)(void*));


#endif
