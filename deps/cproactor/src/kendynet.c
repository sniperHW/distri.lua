#include "kendynet.h"
#include "kn_proactor.h"
#include "kn_fd.h"
#include "kn_connector.h"
#include "kn_acceptor.h"
#include "kn_datasocket.h"
#include "kn_time.h"
#include <assert.h>

int32_t kn_net_open(){
	signal(SIGPIPE,SIG_IGN);
	return 0;
}

static int  kn_set_addrreuse(int fd){
	int32_t yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)))
		return -1;
	return 0;
}

static int kn_bind(int fd,kn_sockaddr *addr_local){
	assert(addr_local);
	int ret = -1;
	if(addr_local->addrtype == AF_INET)
		ret = bind(fd,(struct sockaddr*)&addr_local->in,sizeof(addr_local->in));
	else if(addr_local->addrtype == AF_INET6)
		ret = bind(fd,(struct sockaddr*)&addr_local->in6,sizeof(addr_local->in6));
	else if(addr_local->addrtype == AF_LOCAL)
		ret = bind(fd,(struct sockaddr*)&addr_local->un,sizeof(addr_local->un));
	return ret;	
}

kn_fd_t     kn_listen(kn_proactor_t p,
					  //int protocol,
					  //int type,
					  kn_sockaddr *addr_local,
					  void (*cb_accept)(kn_fd_t,void *ud),
					  void *ud
					  )
{
	assert(p);
	assert(addr_local);
	assert(cb_accept);
	kn_acceptor_t a;
	int ret;
	int family = addr_local->addrtype;
	int fd;
	int protocol;
	
	if(addr_local->addrtype == AF_INET)
		protocol = IPPROTO_TCP;
	else
		protocol = 0;
	if((fd = socket(family,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,protocol)) < 0) 
		return NULL;
	
	if(kn_set_addrreuse(fd) < 0){
		 close(fd);
		 return NULL;
	 }
	
	if(kn_bind(fd,addr_local) < 0){
		 close(fd);
		 return NULL;
	 }
	
	if((ret = listen(fd,SOMAXCONN)) < 0){
		close(fd);
		return NULL;
	}
	
	a = kn_new_acceptor(fd,addr_local,cb_accept,ud);
    if(p->Register(p,(kn_fd_t)a) == 0)
        return (kn_fd_t)a;
    else{
        kn_closefd((kn_fd_t)a);
        return NULL;
    }
}

kn_fd_t 	kn_dgram_listen(struct kn_proactor *p,
					        //int protocol,
					        //int type,
					        kn_sockaddr *addr_local,
					        kn_cb_transfer cb)
{
	assert(p);
	assert(addr_local);
	assert(cb);
	int family = addr_local->addrtype;
	int fd;
	kn_fd_t d;
	int protocol;
	if(addr_local->addrtype == AF_INET)
		protocol = IPPROTO_TCP;
	else
		protocol = 0;	
	if((fd = socket(family,SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC,protocol)) < 0) 
		return NULL;
	
	if(kn_set_addrreuse(fd) < 0){
		 close(fd);
		 return NULL;
	 }
	
	if(kn_bind(fd,addr_local) < 0){
		 close(fd);
		 return NULL;
	 }
	 
	 d = (kn_fd_t)kn_new_datasocket(fd,DGRAM_SOCKET,addr_local,NULL);
	 	 
	 if(0 != kn_proactor_bind(p,d,cb)){
		 kn_closefd(d);
		 return NULL;
	 }else
		return d;
	 	
}

kn_fd_t kn_connect(int type,struct kn_sockaddr *addr_local,struct kn_sockaddr *addr_remote){
	assert(addr_remote);
	int ret;
	int family = addr_remote->addrtype;
    socklen_t len;
    struct kn_sockaddr local;	
	int fd;
	int sock_type;
	kn_fd_t s;
	int protocol;
		
	if(type != SOCK_STREAM && type != SOCK_DGRAM)
		return NULL;
	
	if(addr_remote->addrtype == AF_INET){
		if(type == SOCK_DGRAM) protocol = IPPROTO_UDP;
		else protocol = IPPROTO_TCP;
	}else{
		protocol = 0;
	}	
	
	if((fd = socket(family,type,protocol)) < 0) 
		return NULL;
	
	if(addr_local){
		if(kn_bind(fd,addr_local) < 0){
			close(fd);
			return NULL;
		}
	}
				
	if(family == AF_INET)
		ret = connect(fd,(const struct sockaddr *)&addr_remote->in,sizeof(addr_remote->in));
	else if(family == AF_INET6)
		ret = connect(fd,(const struct sockaddr *)&addr_remote->in6,sizeof(addr_remote->in6));
	else if(family == AF_LOCAL)
		ret = connect(fd,(const struct sockaddr *)&addr_remote->un,sizeof(addr_remote->un));
	else{
		close(fd);
		return NULL;
	}
	if(ret < 0){
		 close(fd);
		 return NULL;
	}
	
	if(!addr_local){		
		local.addrtype = addr_remote->addrtype;
		if(local.addrtype == AF_INET){
			len = sizeof(local.in);
			getsockname(fd,(struct sockaddr*)&local.in,&len);
		}else if(local.addrtype == AF_INET6){
			len = sizeof(local.in6);
			getsockname(fd,(struct sockaddr*)&local.in6,&len);
		}else{
			len = sizeof(local.un);
			getsockname(fd,(struct sockaddr*)&local.un,&len);
		}
		addr_local = &local;
	}
	if(type == SOCK_STREAM) sock_type = STREAM_SOCKET;
	else sock_type = DGRAM_SOCKET;
	s = kn_new_datasocket(fd,sock_type,addr_local,addr_remote);
	kn_set_noblock(s);
	return s;
}

int kn_asyn_connect(kn_proactor_t p,
			   int type,
			   struct kn_sockaddr *addr_local,
			   struct kn_sockaddr *addr_remote,
			   void (*cb_connect)(kn_fd_t,struct kn_sockaddr*,void*,int),
			   void *ud,
			   uint64_t timeout)
{
	assert(p);
	assert(addr_remote);
	assert(cb_connect);
	kn_connector_t c;
	int ret;
	int family = addr_remote->addrtype;
    socklen_t len;
    struct kn_sockaddr local;	
	int fd;
	int protocol;
			
	if(type != SOCK_STREAM)
		return -1;
		
	if(addr_remote->addrtype == AF_INET)
		protocol = IPPROTO_TCP;
	else
		protocol = 0;		
	
	if((fd = socket(family,type|SOCK_NONBLOCK,protocol)) < 0) 
		return -1;

	if(addr_local){
		if(kn_bind(fd,addr_local) < 0){
			close(fd);
			return -1;
		}
	}

	if(family == AF_INET)
		ret = connect(fd,(const struct sockaddr *)&addr_remote->in,sizeof(addr_remote->in));
	else if(family == AF_INET6)
		ret = connect(fd,(const struct sockaddr *)&addr_remote->in6,sizeof(addr_remote->in6));
	else if(family == AF_LOCAL)
		ret = connect(fd,(const struct sockaddr *)&addr_remote->un,sizeof(addr_remote->un));
	else{
		close(fd);
		return -1;
	}
	if(ret < 0 && errno != EINPROGRESS){
		close(fd);
		return -1;
	}
	if(ret == 0){
		
		if(!addr_local){		
			local.addrtype = addr_remote->addrtype;
			if(local.addrtype == AF_INET){
				len = sizeof(local.in);
				getsockname(fd,(struct sockaddr*)&local.in,&len);
			}else if(local.addrtype == AF_INET6){
				len = sizeof(local.in6);
				getsockname(fd,(struct sockaddr*)&local.in6,&len);
			}else{
				len = sizeof(local.un);
				getsockname(fd,(struct sockaddr*)&local.un,&len);
			}
			addr_local = &local;
    	}
		cb_connect(kn_new_datasocket(fd,STREAM_SOCKET,addr_local,addr_remote),addr_remote,ud,0);
		return 0;
	}else{
		c = kn_new_connector(fd,addr_remote,cb_connect,ud,kn_systemms64()+timeout);
        if(p->Register(p,(kn_fd_t)c) == 0){
            return 0;
		}
        else{
            kn_closefd((kn_fd_t)c);
            return -1;
        }		
	}
}

int32_t kn_proactor_run(kn_proactor_t p,int32_t timeout)
{
	assert(p);
	return p->Loop(p,timeout);
}

int32_t kn_proactor_bind(kn_proactor_t p ,kn_fd_t s,kn_cb_transfer cb){
	assert(p);
	assert(s);
	assert(cb);
	struct kn_datasocket *d = (struct kn_datasocket*)s;
	if(s->type == STREAM_SOCKET || s->type == DGRAM_SOCKET){
		if(s->proactor) return 0;
		d->cb_transfer = cb;
		if(0 != p->Register(p,s)){
			return -1;
		}
		return 0;
	}
	return -1;
}


void kn_closefd(kn_fd_t s)
{
	assert(s);
	kn_ref_release(&s->ref);
	/*if(s->proactor)
		s->proactor->UnRegister(s->proactor,s);
	kn_ref_release(&s->ref);*/
}

void kn_shutdown_recv(kn_fd_t s)
{
	assert(s);
	shutdown(s->fd,SHUT_RD);
}

void kn_shutdown_send(kn_fd_t s)
{
	assert(s);
	shutdown(s->fd,SHUT_WR);
}
kn_proactor_t kn_fd_getproactor(kn_fd_t s)
{
	assert(s);
	return s->proactor;
}

void kn_fd_setud(kn_fd_t s,void *ud)
{
	assert(s);
	s->ud = ud;
}

void* kn_fd_getud(kn_fd_t s)
{
	assert(s);
	return s->ud;
}

int32_t kn_recv(kn_fd_t s,st_io *ioreq,int32_t *err_code)
{
	assert(s);
	assert(ioreq);
	struct  kn_datasocket *d = (struct kn_datasocket*)s;
	int32_t  ret;
	int32_t errcode;
	ret = d->raw_recv(d,ioreq,&errcode);
	if(err_code) *err_code = errcode;
	//if(ret == 0) return -1;
	//if(ret < 0 && errcode == EAGAIN) return 0;
	return ret;

}

int32_t kn_send(kn_fd_t s,st_io *ioreq,int32_t *err_code)
{
	assert(s);
	assert(ioreq);	
	struct  kn_datasocket *d = (struct kn_datasocket*)s;
	int32_t ret;
	int32_t errcode;
	ret = d->raw_send(d,ioreq,&errcode);
	if(err_code) *err_code = errcode;
	//if(ret == 0) return -1;
	//if(ret < 0 && errcode == EAGAIN) return 0;
	return ret;

}

int32_t kn_post_recv(kn_fd_t s,st_io *ioreq)
{
	assert(s);
	assert(ioreq);		
	struct  kn_datasocket *d = (struct kn_datasocket*)s;
	kn_list_pushback(&d->pending_recv,(kn_list_node*)ioreq);
	if(s->proactor && d->flag & readable)
		kn_procator_putin_active(s->proactor,s);

	return 0;
}

int32_t kn_post_send(kn_fd_t s,st_io *ioreq)
{
	assert(s);
	assert(ioreq);		
	struct  kn_datasocket *d = (struct kn_datasocket*)s;
	kn_list_pushback(&d->pending_send,(kn_list_node*)ioreq);
	if(s->proactor && d->flag & writeable)
		kn_procator_putin_active(s->proactor,s);

	return 0;
}

void kn_fd_set_stio_destroyer(kn_fd_t s,stio_destroyer d)
{
	assert(s);
	assert(d);
	if(s->type == STREAM_SOCKET || s->type == DGRAM_SOCKET){
		((kn_datasocket*)s)->destroy_stio = d;
	}
}

kn_sockaddr*  kn_fd_get_local_addr(kn_fd_t s){
	assert(s);	
	if(s->type == STREAM_SOCKET || s->type == DGRAM_SOCKET){
		return &((kn_datasocket*)s)->addr_local;
	}
	return NULL;
}

int kn_fd_get_type(kn_fd_t s)
{
	assert(s);	
	return s->type;
}
