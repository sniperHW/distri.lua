#include "kn_timerfd.h"
#include "kn_proactor.h"
#include <sys/timerfd.h> 
void kn_timerfd_on_active(kn_fd_t s,int event){
	long long tmp;
	read(s->fd,&tmp,sizeof(tmp));
	kn_timermgr_tick(s->proactor->timermgr);
}

void kn_timerfd_destroy(void *ptr){
	kn_timerfd_t t = (kn_timerfd_t)((char*)ptr - sizeof(kn_dlist_node));
	close(t->base.fd);
	free(t);
}

kn_fd_t kn_new_timerfd(uint32_t timeout){
	kn_timerfd_t fd = calloc(1,sizeof(*fd));
	fd->base.fd = timerfd_create(CLOCK_MONOTONIC,0);
	if(fd->base.fd < 0){
		free(fd);
		return NULL;
	}
	struct itimerspec spec;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    int sec = timeout/1000;
    int ms = timeout%1000;    
    long long nosec = (now.tv_sec + sec)*1000*1000*1000 + now.tv_nsec + ms*1000*1000;
	spec.it_value.tv_sec = nosec/(1000*1000*1000);
    spec.it_value.tv_nsec = nosec%(1000*1000*1000);
    spec.it_interval.tv_sec = sec;
    spec.it_interval.tv_nsec = ms*1000*1000;	
	
	if(0 != timerfd_settime(fd->base.fd,TFD_TIMER_ABSTIME,&spec,0))
	{
		close(fd->base.fd);
		free(fd);
		return NULL;
	}
	fd->base.on_active = kn_timerfd_on_active;
	kn_ref_init(&fd->base.ref,kn_timerfd_destroy);   
	fd->base.type = TIMER_FD;
	return (kn_fd_t)fd;
}
