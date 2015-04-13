#include "kn_refobj.h"
#include "kn_time.h"
#include "kn_common_define.h"

volatile uint32_t g_ref_counter = 0;

void refobj_init(refobj *r,void (*destructor)(void*))
{
	r->destructor = destructor;
	r->high32 = kn_systemms();
	r->low32  = (uint32_t)(ATOMIC_INCREASE(&g_ref_counter));
	ATOMIC_INCREASE(&r->refcount);
}

uint32_t refobj_dec(refobj *r)
{
    volatile uint32_t count;
    uint32_t c;
    struct timespec ts;
    assert(r->refcount > 0);
    if((count = ATOMIC_DECREASE(&r->refcount)) == 0){
        for(c = 0,r->identity = 0;;){
            if(COMPARE_AND_SWAP(&r->flag,0,1))
                break;
            if(++c < 4000){
                __asm__("pause");
            }else{
                c = 0;
                ts.tv_sec = 0;
                ts.tv_nsec = 500000;
                nanosleep(&ts, NULL);
            }
        }
        r->destructor(r);
    }
    return count;
}

refobj *cast2refobj(ident _ident)
{
    refobj *ptr = NULL;
    if(unlikely(!_ident.ptr)) return NULL;
    TRY{
              refobj *o = (refobj*)_ident.ptr;
              uint32_t c = 0;
    	struct timespec ts;
              while(_ident.identity == o->identity){
                    if(COMPARE_AND_SWAP(&o->flag,0,1)){
                        if(_ident.identity == o->identity){
                                if(__sync_fetch_and_add(&o->refcount,1) > 0)
                                    ptr = o;
                                else
                                    o->refcount = 0;
                        }
                        o->flag = 0;
                        break;
                    }else{                     
                     if(++c < 4000){
                         __asm__("pause");
                     }else{
                        c = 0;
                        ts.tv_sec = 0;
                        ts.tv_nsec = 500000;
                        nanosleep(&ts, NULL);
                    }
                }                
              }
    }CATCH_ALL{
            ptr = NULL;      
    }ENDTRY;
    return ptr; 
}    

