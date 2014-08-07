#ifndef _KN_PROACTOR_H
#define _KN_PROACTOR_H

#include <stdint.h>
#include "kn_list.h"
#include "kn_dlist.h"
#include "kn_common_include.h"
#include "kn_timer.h"

struct kn_fd;

kn_timermgr_t kn_new_timermgr();
void          kn_del_timermgr(kn_timermgr_t);
void          kn_timermgr_tick(kn_timermgr_t);

typedef struct kn_proactor
{
    int32_t    (*Loop)(struct kn_proactor*,int32_t timeout);
    int32_t    (*Register)(struct kn_proactor*,struct kn_fd*);
    int32_t    (*UnRegister)(struct kn_proactor*,struct kn_fd*);
    //private use for redisconn
    int32_t    (*addRead)(struct kn_proactor*,struct kn_fd*);
    int32_t    (*delRead)(struct kn_proactor*,struct kn_fd*);
    int32_t    (*addWrite)(struct kn_proactor*,struct kn_fd*);
    int32_t    (*delWrite)(struct kn_proactor*,struct kn_fd*);
	kn_timer_t (*reg_timer)(struct kn_proactor*,uint64_t timeout,kn_cb_timer cb,void *ud);
    
    kn_dlist actived[2];
    int8_t   actived_index;
    kn_dlist connecting;
	kn_timermgr_t timermgr;
}kn_proactor,*kn_proactor_t;


kn_proactor_t kn_new_proactor();
void          kn_close_proactor(kn_proactor_t);


static inline kn_dlist* kn_proactor_activelist(kn_proactor_t p){
	return &p->actived[p->actived_index];
}

static inline void kn_procator_putin_active(kn_proactor_t p,struct kn_fd *s)
{
    kn_dlist *current_active = &p->actived[p->actived_index];
    kn_dlist_push(current_active,(kn_dlist_node*)s);
}

#endif
