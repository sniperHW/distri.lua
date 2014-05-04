#include "kn_acceptor.h"
#include "kn_proactor.h"
#include "kn_datasocket.h"


static void kn_acceptor_destroy(void *ptr){
	kn_acceptor_t a = (kn_acceptor_t)((char*)ptr - sizeof(kn_dlist_node));;
	if(a->base.fd >= 0) close(a->base.fd);
	free(a);
}


static int _accept(kn_acceptor_t a,kn_sockaddr *remote){
	int fd;
	socklen_t len;
	int family = a->addr_local.addrtype;
again:
	if(family == AF_INET){
		len = sizeof(remote->in);
		fd = accept(a->base.fd,(struct sockaddr*)&remote->in,&len);
	}else if(family == AF_INET6){
		len = sizeof(remote->in6);
		fd = accept(a->base.fd,(struct sockaddr*)&remote->in6,&len);
	}else if(family == AF_LOCAL){
		len = sizeof(remote->un);
		fd = accept(a->base.fd,(struct sockaddr*)&remote->un,&len);
	}else{
		return -1;
	}

	if(fd < 0){
#ifdef EPROTO
		if(errno == EPROTO || errno == ECONNABORTED)
#else
		if(errno == ECONNABORTED)
#endif
			goto again;
		else
			return -errno;
	}
	return fd;
}

int kn_set_noblock(kn_fd_t);

static void kn_acceptor_on_active(struct kn_fd *s,int event){
	kn_acceptor_t a = (kn_acceptor_t)s;
    kn_fd_t t;
    int fd;
    kn_sockaddr remote;
    while(1)
    {
    	fd = _accept(a,&remote);
    	if(fd < 0)
    		break;
    	else{
    		t = kn_new_datasocket(fd,STREAM_SOCKET,&a->addr_local,&remote);
    		kn_set_noblock(t);
    		a->cb_accept(t,s->ud);
    	}      
    }
}

kn_acceptor_t kn_new_acceptor(int fd,kn_sockaddr *local,void (*cb_accept)(kn_fd_t,void*),void *ud){
	kn_acceptor_t a = calloc(1,sizeof(*a));
	a->base.fd = fd;
	kn_ref_init(&a->base.ref,kn_acceptor_destroy);
	a->base.type = ACCEPTOR;
	a->base.on_active = kn_acceptor_on_active;
	a->addr_local = *local;
	a->cb_accept = cb_accept;
	a->base.ud = ud;
	return a;
}
