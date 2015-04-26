#ifndef _KN_TIMER_PRIVATE_H
#define _KN_TIMER_PRIVATE_H

/*typedef struct kn_timermgr *kn_timermgr_t;

kn_timermgr_t kn_new_timermgr();

kn_timer_t reg_timer_imp(kn_timermgr_t t,uint64_t timeout,int32_t(*)(uint64_t,void*),void *ud);

void kn_timermgr_tick(kn_timermgr_t t);

void kn_del_timermgr(kn_timermgr_t t);
*/


typedef struct wheelmgr *wheelmgr_t;

wheelmgr_t wheelmgr_new();
void       wheelmgr_del(wheelmgr_t);
kn_timer_t wheelmgr_register(wheelmgr_t,uint32_t timeout,int32_t(*)(uint32_t,void*),void*);
void       kn_del_timer(kn_timer_t);
void       wheelmgr_tick(wheelmgr_t,uint64_t now); 

#endif
