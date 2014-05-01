#include "kn_proactor.h"
#include "kn_epoll.h"

int kn_max_proactor_fd = 65536; 

kn_proactor_t kn_new_proactor(){	
	return (kn_proactor_t)kn_epoll_new();
}

void kn_close_proactor(kn_proactor_t p){
	kn_epoll_del((kn_epoll*)p);
}
