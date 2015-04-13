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
#ifndef _KN_REFOBJ_H
#define _KN_REFOBJ_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <assert.h>
#include "kn_atomic.h"
#include "kn_except.h"  
  
typedef  void (*refobj_destructor)(void*);

typedef struct refobj
{
        volatile uint32_t refcount;
        uint32_t               pad1;
        volatile uint32_t flag;  
        uint32_t               pad2;      
        union{
            struct{
                volatile uint32_t low32; 
                volatile uint32_t high32;       
            };
            volatile uint64_t identity;
        };
        void (*destructor)(void*);
}refobj;

void refobj_init(refobj *r,void (*destructor)(void*));

static inline uint32_t refobj_inc(refobj *r)
{
    return ATOMIC_INCREASE(&r->refcount);
}

uint32_t refobj_dec(refobj *r);

typedef struct{
    union{  
        struct{ 
            uint64_t identity;    
            refobj   *ptr;
        };
        uint32_t _data[4];
    };
}ident;

refobj *cast2refobj(ident _ident);

static inline ident make_ident(refobj *ptr)
{
    return (ident){.identity=ptr->identity,.ptr=ptr};
}

static const ident empty_ident = {.identity=0,.ptr=NULL};

static inline void make_empty_ident(ident *_ident)
{
    *_ident = empty_ident;
}

static inline int is_empty_ident(ident _ident){
    return _ident.ptr == NULL ? 1 : 0;
}

#endif
