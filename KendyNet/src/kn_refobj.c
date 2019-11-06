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
    assert(r->refcount > 0);
    if((count = ATOMIC_DECREASE(&r->refcount)) == 0){
        r->identity = 0;//立即清0,使得cast2refobj可以快速查觉
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
        /*
         *   假设refcount == 1
         *   A线程执行到COMPARE_AND_SWAP的前一条指令然后暂停,     
         *   B执行ATOMIC_DECREASE,refcount == 0,此时无论r->identity = 0是否执行。当A恢复执行后最终必将会发现oldCount == 0跳出for循环
         *
         */
        for(;_ident.identity == o->identity;) {
            uint32_t oldCount = o->refcount;
            uint32_t newCount = oldCount + 1;
            if(oldCount == 0) {
                break;
            }
            if(COMPARE_AND_SWAP(&o->refcount,oldCount,newCount)){
                ptr = o;
            }    
        }
    }CATCH_ALL{
            ptr = NULL;      
    }ENDTRY;
    return ptr; 
}    

