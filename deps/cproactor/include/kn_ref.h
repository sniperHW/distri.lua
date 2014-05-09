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
#ifndef _KN_REF_H
#define _KN_REF_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "kn_atomic.h"
#include "kn_except.h"  
  
typedef  void (*kn_ref_destroyer)(void*);

typedef struct kn_ref
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
}kn_ref;

void kn_ref_init(kn_ref *r,void (*destroyer)(void*));

static inline atomic_32_t kn_ref_acquire(kn_ref *r)
{
    return ATOMIC_INCREASE(&r->refcount);
}

static inline atomic_32_t kn_ref_release(kn_ref *r)
{
    atomic_32_t count;
    int c;
    struct timespec ts;
    if((count = ATOMIC_DECREASE(&r->refcount)) == 0){
        r->identity = 0;
        _FENCE;
        c = 0;
        for(;;){
            if(COMPARE_AND_SWAP(&r->flag,0,1))
                break;
            if(c < 4000){
                ++c;
                __asm__("pause");
            }else{
                ts.tv_sec = 0;
                ts.tv_nsec = 500000;
                nanosleep(&ts, NULL);
            }
        }
        r->destroyer(r);
    }
    return count;
}


typedef struct ident{
	union{	
		struct{ 
			uint64_t identity;    
			kn_ref   *ptr;
		};
		uint32_t _data[4];
	};
}ident;

static inline ident make_ident(kn_ref *ptr)
{
	ident _ident = {.identity=ptr->identity,.ptr=ptr};
    return _ident;
}

static inline kn_ref *cast2ref(ident _ident)
{
    kn_ref *ptr = NULL;
    if(!_ident.ptr) return NULL;
    TRY{    
        while(_ident.identity == _ident.ptr->identity)
        {
            if(COMPARE_AND_SWAP(&_ident.ptr->flag,0,1))
            {
                
                if(_ident.identity == _ident.ptr->identity &&
                   kn_ref_acquire(_ident.ptr) > 0)
                        ptr = _ident.ptr;
                _FENCE;
                _ident.ptr->flag = 0;
                break;
            }
        }
    }CATCH_ALL
    {
        ptr = NULL;      
    }ENDTRY;
    return ptr; 
}
/*
static inline void make_empty_ident(ident *_ident)
{
    _ident->identity = 0;
    _ident->ptr = NULL;
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
}
*/
#endif
