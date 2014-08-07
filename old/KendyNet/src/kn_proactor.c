#include "kn_proactor.h"
#include "kn_epoll.h"

int kn_max_proactor_fd = 65536; 

kn_proactor_t kn_new_proactor(){	
	return (kn_proactor_t)kn_epoll_new();
}

void kn_close_proactor(kn_proactor_t p){
	kn_epoll_del((kn_epoll*)p);
}
kn_timer_t _kn_reg_timer(kn_timermgr_t t,uint64_t timeout,kn_cb_timer cb,void *ud);

kn_timer_t kn_proactor_reg_timer(kn_proactor_t p,uint64_t timeout,kn_cb_timer cb,void *ud){
	return _kn_reg_timer(p->timermgr,timeout,cb,ud);
}
