#include "kn_datagram_socket.h"



static int datagram_socket_associate(engine_t e,
		               handle_t h,
		               void (*callback)(handle_t,void*,int,int)){
	if(((handle_t)h)->type != KN_SOCKET) return -1;						  
	kn_socket *s = (kn_socket*)h;
	if(s->e) kn_event_del(s->e,h);
	s->callback = callback;
	s->e = e;
	kn_set_noblock(h->fd,0);
#ifdef _LINUX
	kn_event_add(s->e,h,EPOLLIN);
	kn_disable_read(s->e,h);	
#elif   _BSD
	kn_event_add(s->e,h,EVFILT_READ);
	kn_disable_read(s->e,h);
#endif
	return 0;
}

static void on_events(handle_t h,int events);

static void on_destroy(void *_){
	kn_socket *s = (kn_socket*)_;
	st_io *io_req;
	if(s->clear_func){
        		while((io_req = (st_io*)kn_list_pop(&s->pending_send))!=NULL)
            			s->clear_func(io_req);		
        		while((io_req = (st_io*)kn_list_pop(&s->pending_recv))!=NULL)
            			s->clear_func(io_req);
	}
	close(s->comm_head.fd);
	free(s);
}

handle_t new_datagram_socket(int fd,int domain){
	kn_datagram_socket *ds = calloc(1,sizeof(*ds));
	if(!ds){
		return NULL;
	}	
	((handle_t)ds)->fd = fd;
	((handle_t)ds)->type = KN_SOCKET;
	((handle_t)ds)->status = SOCKET_DATAGRAM;
	((kn_socket*)ds)->domain = domain;
	((kn_socket*)ds)->type = SOCK_DGRAM;
	((handle_t)ds)->on_events = on_events;
	//((handle_t)ds)->on_destroy = on_destroy;
	((handle_t)ds)->associate = datagram_socket_associate;
	return (handle_t)ds; 
}

static inline struct sockaddr* fetch_addr(kn_sockaddr *_addr){
	if(_addr->addrtype == AF_INET)
		return (struct sockaddr*)&_addr->in;
	else if(_addr->addrtype == AF_INET6)
		return (struct sockaddr*)&_addr->in6;
	else if(_addr->addrtype == AF_LOCAL)
		return (struct sockaddr*)&_addr->un;
	return NULL;
}

static inline socklen_t fetch_addr_len(kn_sockaddr *_addr){
	if(_addr->addrtype == AF_INET)
		return sizeof(_addr->in);
	else if(_addr->addrtype == AF_INET6)
		return sizeof(_addr->in6);
	else if(_addr->addrtype == AF_LOCAL)
		return sizeof(_addr->un);
	return 0;
}

static void process_read(kn_socket *s){
	st_io* io_req = 0;
	int bytes_transfer = 0;
	struct msghdr _msghdr;
	while((io_req = (st_io*)kn_list_pop(&s->pending_recv))!=NULL){
		errno = 0;
		_msghdr = (struct msghdr){
			.msg_name = &io_req->addr,
			.msg_namelen = sizeof(io_req->addr),
			.msg_iov = io_req->iovec,
			.msg_iovlen = io_req->iovec_count,
			.msg_flags = 0,
			.msg_control = NULL,
			.msg_controllen = 0
		};
		io_req->recvflags = 0;
		bytes_transfer = TEMP_FAILURE_RETRY(recvmsg(s->comm_head.fd,&_msghdr,0));	
		io_req->recvflags = _msghdr.msg_flags;
		if(bytes_transfer < 0 && errno == EAGAIN){
				//将请求重新放回到队列
				kn_list_pushback(&s->pending_recv,(kn_list_node*)io_req);
				break;
		}else{
			s->callback((handle_t)s,io_req,bytes_transfer,errno);
			if(s->comm_head.status == SOCKET_CLOSE)
				return;			
		}
	}	
	if(!kn_list_size(&s->pending_recv)){
		//没有接收请求了,取消EPOLLIN
		kn_disable_read(s->e,(handle_t)s);
	}	
}

static void process_write(kn_socket *s){
	st_io* io_req = 0;
	int bytes_transfer = 0;
	struct msghdr _msghdr;
	while((io_req = (st_io*)kn_list_pop(&s->pending_send))!=NULL){
		errno = 0;
		_msghdr = (struct msghdr){
			.msg_name = &io_req->addr,
			.msg_namelen = sizeof(io_req->addr),
			.msg_iov = io_req->iovec,
			.msg_iovlen = io_req->iovec_count,
			.msg_flags = 0,
			.msg_control = NULL,
			.msg_controllen = 0
		};
		bytes_transfer = TEMP_FAILURE_RETRY(sendmsg(s->comm_head.fd,&_msghdr,0));	
		s->callback((handle_t)s,io_req,bytes_transfer,errno);
		if(s->comm_head.status == SOCKET_CLOSE)
			return;			
	}	
	if(!kn_list_size(&s->pending_send)){
		kn_disable_write(s->e,(handle_t)s);
	}	
}

static void on_events(handle_t h,int events){	
	kn_socket *s = (kn_socket*)h;
	if(h->status == SOCKET_CLOSE)
		return;
	do{
		h->inloop = 1;
		if(h->status == SOCKET_DATAGRAM){
			if(events & EVENT_READ){
				process_read(s);	
				if(h->status == SOCKET_CLOSE) 
					break;								
			}
			if(events & EVENT_WRITE){
				process_write(s);
			}				
		}
		h->inloop = 0;
	}while(0);
	if(h->status == SOCKET_CLOSE)
		on_destroy(s);
}

int datagram_socket_close(handle_t h){
	if(h->type != KN_SOCKET && ((kn_socket*)h)->type != SOCK_DGRAM)
		return -1;
	kn_socket *s = (kn_socket*)h;
	if(h->status != SOCKET_CLOSE){
		if(h->inloop){
			h->status = SOCKET_CLOSE;
			//kn_push_destroy(s->e,h);
		}else
			on_destroy(s);				
		return 0;
	}
	return -1;		
}

static int _bind(int fd,kn_sockaddr *addr_local){
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

int datagram_socket_listen(kn_datagram_socket *ds,kn_sockaddr *local){
	int32_t yes = 1;
	if(setsockopt(((handle_t)ds)->fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)))
		return -1;	
	return _bind(((handle_t)ds)->fd,local);
}

int datagram_socket_connect(kn_datagram_socket *ds,kn_sockaddr *local,kn_sockaddr *remote){
	return -1;
/*	int fd = ((handle_t)ds)->fd;
	socklen_t len;	
	if(local){
		if(_bind(fd,local) < 0){
			return -1;
		}
	}
	int ret;
	if(((kn_socket*)ss)->domain == AF_INET)
		ret = connect(fd,(const struct sockaddr *)&remote->in,sizeof(remote->in));
	else if(((kn_socket*)ss)->domain == AF_INET6)
		ret = connect(fd,(const struct sockaddr *)&remote->in6,sizeof(remote->in6));
	else if(((kn_socket*)ss)->domain == AF_LOCAL)
		ret = connect(fd,(const struct sockaddr *)&remote->un,sizeof(remote->un));
	else{
		return -1;
	}
	if(ret == 0){		
		if(!local){		
			((kn_socket*)ss)->addr_local.addrtype = ((kn_socket*)ss)->domain;
			if(((kn_socket*)ss)->addr_local.addrtype == AF_INET){
				len = sizeof(((kn_socket*)ds)->addr_local.in);
				getsockname(fd,(struct sockaddr*)&((kn_socket*)ds)->addr_local.in,&len);
			}else if(((kn_socket*)ss)->addr_local.addrtype == AF_INET6){
				len = sizeof(((kn_socket*)ds)->addr_local.in6);
				getsockname(fd,(struct sockaddr*)&((kn_socket*)ds)->addr_local.in6,&len);
			}else{
				len = sizeof(((kn_socket*)ds)->addr_local.un);
				getsockname(fd,(struct sockaddr*)&((kn_socket*)ds)->addr_local.un,&len);
			}
    		}
		return 1;
	}
	return -1;*/		
}

