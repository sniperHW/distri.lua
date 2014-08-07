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
#ifndef _UTHREAD_H
#define _UTHREAD_H
#include <stdint.h>
typedef struct uthread *uthread_t;

uthread_t uthread_create(uthread_t parent,void*stack,uint32_t stack_size,void*(*fun)(void*));
void uthread_destroy(uthread_t*);
void* __attribute__((regparm(3))) uthread_switch(uthread_t from,uthread_t to,void *para);



#endif
