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
#ifndef _REFBASE_H
#define _REFBASE_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "atomic.h"
#include "except.h"
#include <signal.h>    

struct refbase
{
        atomic_32_t refcount;
        union{
            struct{
                uint32_t low32; 
                uint32_t high32;       
            };
            atomic_64_t identity;
        };
        atomic_32_t flag;
        void (*destroyer)(void*);
};

void ref_init(struct refbase *r,uint16_t type,void (*destroyer)(void*),int32_t initcount);

static inline atomic_32_t ref_increase(struct refbase *r)
{
    return ATOMIC_INCREASE(&r->refcount);
}

static inline atomic_32_t ref_decrease(struct refbase *r)
{
    atomic_32_t count;
    if((count = ATOMIC_DECREASE(&r->refcount)) == 0){
                r->identity = 0;
                _FENCE;
        int32_t c = 0;
        for(;;){
            if(COMPARE_AND_SWAP(&r->flag,0,1))
                break;
            if(c < 4000){
                ++c;
                __asm__("pause");
            }else{
                struct timespec ts = { 0, 500000 };
                nanosleep(&ts, NULL);
            }
        }
                r->destroyer(r);
        }
    return count;
}

typedef struct ident
{
    uint64_t identity;    
    struct refbase *ptr;
}ident;

static inline ident make_ident(struct refbase *ptr)
{
        ident _ident = {ptr->identity,ptr};
        return _ident;
}

static inline ident make_empty_ident()
{
    ident _ident = {0,NULL};
        return _ident;
}

static inline struct refbase *cast_2_refbase(ident _ident)
{
    struct refbase *ptr = NULL;
    TRY{    
        while(_ident.identity == _ident.ptr->identity)
        {
            if(COMPARE_AND_SWAP(&_ident.ptr->flag,0,1))
            {
                
                if(_ident.identity == _ident.ptr->identity &&
                   ref_increase(_ident.ptr) > 0)
                        ptr = _ident.ptr;
                _FENCE;
                _ident.ptr->flag = 0;
                break;
            }
        }
    }CATCH_ALL
    {
		//出现异常表示_ident.ptr已被释放，直接返回NULL
        //printf("catch error,%x\n",_ident.ptr);
        ptr = NULL;      
    }ENDTRY;
    return ptr;
/*
    struct refbase *ptr = NULL; 
    while(_ident.identity == _ident.ptr->identity)
    {
        if(COMPARE_AND_SWAP(&_ident.ptr->flag,0,1))
        {
            
            if(_ident.identity == _ident.ptr->identity &&
               ref_increase(_ident.ptr) > 0)
                    ptr = _ident.ptr;
            _FENCE;
            _ident.ptr->flag = 0;
            break;
        }
    }
return ptr;*/    
}

static inline int32_t is_vaild_ident(ident _ident)
{
        if(!_ident.ptr || !_ident.identity) return 0;
        return 1;
}

static inline int8_t eq_ident(ident a,ident b)
{
    return a.identity == b.identity && a.ptr == b.ptr;
}

static inline int8_t is_type(ident a,uint16_t type)
{
    uint32_t low32 = (uint32_t)a.identity;
    uint32_t _type = type*65536;
    if(low32 & _type) return 1;
    return 0;
    //if(((uint16_t*)&a.identity)[3] == type)
    //    return 1;
    //return 0;
}

#define TO_IDENT(OTHER_IDENT) (*(ident*)&OTHER_IDENT)

#endif
