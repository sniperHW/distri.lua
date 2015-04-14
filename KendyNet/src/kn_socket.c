#include "kn_type.h"
#include "kn_list.h"
#include "kendynet_private.h"
#include <assert.h>
#include <arpa/inet.h>
#include "kn_event.h"
#include "kn_stream_socket.h"
#include "kn_datagram_socket.h"

handle_t kn_new_sock(int domain,int type,int protocal){	
	int fd = socket(domain,type|SOCK_CLOEXEC,protocal);
	if(fd < 0) return NULL;
	handle_t h = NULL;
	if(type == SOCK_STREAM) 
		h = new_stream_socket(fd,domain);
	else if(type == SOCK_DGRAM){
		h = new_datagram_socket(fd,domain);
	}
	if(!h) close(fd);
	return h;
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

static  inline int stream_socket_send(handle_t h,st_io *req){
	kn_socket *s = (kn_socket*)h;
	errno = 0;
	if(!s->e || h->status != SOCKET_ESTABLISH){
		return -1;
	 }	
	if(0 != kn_list_size(&s->pending_send))
		return kn_sock_post_send(h,req);			
	int bytes_transfer = 0;

	if(((kn_stream_socket*)s)->ssl){			
		bytes_transfer = TEMP_FAILURE_RETRY(SSL_write(((kn_stream_socket*)s)->ssl,req->iovec[0].iov_base,req->iovec[0].iov_len));
		int ssl_error = SSL_get_error(((kn_stream_socket*)s)->ssl,bytes_transfer);
		if(bytes_transfer < 0 && (ssl_error == SSL_ERROR_WANT_WRITE ||
					ssl_error == SSL_ERROR_WANT_READ ||
					ssl_error == SSL_ERROR_WANT_X509_LOOKUP)){
			errno = EAGAIN;
		}		
	}
	else	
		bytes_transfer = TEMP_FAILURE_RETRY(writev(h->fd,req->iovec,req->iovec_count));				

		
	if(bytes_transfer < 0 && errno == EAGAIN)
		return kn_sock_post_send(h,req);				
	return bytes_transfer > 0 ? bytes_transfer:-1;
}



static inline int datagam_socket_send(handle_t h,st_io *req){
	kn_socket *s = (kn_socket*)h;
	errno = 0;
	if(h->status != SOCKET_DATAGRAM){
		return -1;
	 }
	if(0 != kn_list_size(&s->pending_send))
		return kn_sock_post_recv(h,req);		 	
	struct msghdr _msghdr = {
		.msg_name = &req->addr,
		.msg_namelen = sizeof(req->addr),
		.msg_iov = req->iovec,
		.msg_iovlen = req->iovec_count,
		.msg_flags = 0,
		.msg_control = NULL,
		.msg_controllen = 0
	};		
	return TEMP_FAILURE_RETRY(sendmsg(s->comm_head.fd,&_msghdr,0));					
}

int kn_sock_send(handle_t h,st_io *req){
	if(h->type != KN_SOCKET){ 
		return -1;
	}	
	if(((kn_socket*)h)->type == SOCK_STREAM)
	 	return stream_socket_send(h,req);
	else if(((kn_socket*)h)->type == SOCK_DGRAM)
		return datagam_socket_send(h,req);
	 else	
	 	return -1;		
}

static inline int stream_socket_recv(handle_t h,st_io *req){
	kn_socket *s = (kn_socket*)h;
	errno = 0;		
	if(!s->e || h->status != SOCKET_ESTABLISH){
		return -1;
	 }	
	if(0 != kn_list_size(&s->pending_recv))
		return kn_sock_post_recv(h,req);
		
	int bytes_transfer = 0;

	if(((kn_stream_socket*)s)->ssl){
		bytes_transfer = TEMP_FAILURE_RETRY(SSL_read(((kn_stream_socket*)s)->ssl,req->iovec[0].iov_base,req->iovec[0].iov_len));
		int ssl_error = SSL_get_error(((kn_stream_socket*)s)->ssl,bytes_transfer);
		if(bytes_transfer < 0 && (ssl_error == SSL_ERROR_WANT_WRITE ||
					ssl_error == SSL_ERROR_WANT_READ ||
					ssl_error == SSL_ERROR_WANT_X509_LOOKUP)){
			errno = EAGAIN;
		}		
	}
	else	
		bytes_transfer = TEMP_FAILURE_RETRY(readv(h->fd,req->iovec,req->iovec_count));				

		
	if(bytes_transfer < 0 && errno == EAGAIN)
		return kn_sock_post_recv(h,req);				
	return bytes_transfer > 0 ? bytes_transfer:-1;		
}

static inline int datagam_socket_recv(handle_t h,st_io *req){
	kn_socket *s = (kn_socket*)h;
	errno = 0;
	if(!s->e || h->status != SOCKET_DATAGRAM){
		return -1;
	 }	
	if(0 != kn_list_size(&s->pending_recv))
		return kn_sock_post_recv(h,req);			

	struct msghdr _msghdr = {
		.msg_name = &req->addr,
		.msg_namelen = sizeof(req->addr),
		.msg_iov = req->iovec,
		.msg_iovlen = req->iovec_count,
		.msg_flags = 0,
		.msg_control = NULL,
		.msg_controllen = 0
	};
	req->recvflags = 0;		
	int bytes_transfer = TEMP_FAILURE_RETRY(recvmsg(s->comm_head.fd,&_msghdr,0));					
	req->recvflags = _msghdr.msg_flags;
	if(bytes_transfer < 0 && errno == EAGAIN)
		return kn_sock_post_recv(h,req);					
	return bytes_transfer > 0 ? bytes_transfer:-1;	
} 

int kn_sock_recv(handle_t h,st_io *req){
	if(h->type != KN_SOCKET){ 
		return -1;
	}	
	if(((kn_socket*)h)->type == SOCK_STREAM)
		return stream_socket_recv(h,req);
	else if(((kn_socket*)h)->type == SOCK_DGRAM)
		return datagam_socket_recv(h,req);
	else
		return -1;
}

static inline int stream_socket_post_send(handle_t h,st_io *req){
	kn_socket *s = (kn_socket*)h;
	if(!s->e || h->status != SOCKET_ESTABLISH){
		return -1;
	 }			 
	if(!is_set_write(h)){
	 	if(0 != kn_enable_write(s->e,h))
	 		return -1;
	}
	kn_list_pushback(&s->pending_send,(kn_list_node*)req);	 	
	return 0;	
}

static inline int datagram_socket_post_send(handle_t h,st_io *req){
	kn_socket *s = (kn_socket*)h;
	if(!s->e || h->status != SOCKET_DATAGRAM){
		return -1;
	 }		 
	 if(!is_set_write(h)){
	 	if(0 != kn_enable_write(s->e,h))
	 		return -1;
	 }	
	kn_list_pushback(&s->pending_send,(kn_list_node*)req);	 	
	return 0;	
}

int kn_sock_post_send(handle_t h,st_io *req){
	if(h->type != KN_SOCKET){ 
		return -1;
	}	
	if(((kn_socket*)h)->type == SOCK_STREAM)
		return stream_socket_post_send(h,req);
	else if(((kn_socket*)h)->type == SOCK_DGRAM)
		return datagram_socket_post_send(h,req);
	else
		return -1;
}

static inline int stream_socket_post_recv(handle_t h,st_io *req){
	kn_socket *s = (kn_socket*)h;
	if(!s->e || h->status != SOCKET_ESTABLISH){
		return -1;
	}	
	 if(!is_set_read(h)){
	 	if(0 != kn_enable_read(s->e,h))
	 		return -1;
	 }	
	kn_list_pushback(&s->pending_recv,(kn_list_node*)req);		
	return 0;		
}

static inline int datagram_socket_post_recv(handle_t h,st_io *req){
	kn_socket *s = (kn_socket*)h;
	if(!s->e || h->status != SOCKET_DATAGRAM){
		return -1;
	 }		 
	 if(!is_set_read(h)){
	 	if(0 != kn_enable_read(s->e,h))
	 		return -1;
	 }	
	kn_list_pushback(&s->pending_recv,(kn_list_node*)req);	 	
	return 0;	
}

int kn_sock_post_recv(handle_t h,st_io *req){
	if(h->type != KN_SOCKET){ 
		return -1;
	}	
	if(((kn_socket*)h)->type == SOCK_STREAM)
		return stream_socket_post_recv(h,req);
	else if(((kn_socket*)h)->type == SOCK_DGRAM)
		return datagram_socket_post_recv(h,req);
	else
		return -1;		
}


int kn_sock_listen(handle_t h,
		   kn_sockaddr *local)
{
	if(!(((handle_t)h)->type & KN_SOCKET)) return -1;	
	kn_socket *s = (kn_socket*)h;
	if(s->type == SOCK_STREAM){
		return stream_socket_listen((kn_stream_socket*)s,local);	
	}else if(s->type == SOCK_DGRAM){
		return datagram_socket_listen((kn_datagram_socket*)s,local);
	}	
	return -1;		
}


int kn_sock_connect(handle_t h,
		      kn_sockaddr *remote,
		      kn_sockaddr *local)
{
	if(((handle_t)h)->type != KN_SOCKET) return -1;
	kn_socket *s = (kn_socket*)h;
	if(s->comm_head.status != SOCKET_NONE) return -1;
	if(s->type == SOCK_STREAM){
		return stream_socket_connect((kn_stream_socket*)s,local,remote);	
	}else if(s->type == SOCK_DGRAM){
		return datagram_socket_connect((kn_datagram_socket*)s,local,remote);
	}
	return -1;	
}

void   kn_sock_set_clearfunc(handle_t h,void (*func)(void*)){
	if(((handle_t)h)->type != KN_SOCKET) 
		return;
	kn_socket *s = (kn_socket*)h;
	s->clear_func = func;	
}

void kn_sock_setud(handle_t h,void *ud){
	if(((handle_t)h)->type != KN_SOCKET) return;
	((handle_t)h)->ud = ud;	
}

void* kn_sock_getud(handle_t h){
	if(((handle_t)h)->type != KN_SOCKET) return NULL;	
	return ((handle_t)h)->ud;
}

int kn_sock_fd(handle_t h){
	return ((handle_t)h)->fd;
}

kn_sockaddr* kn_sock_addrlocal(handle_t h){
	if(((handle_t)h)->type != KN_SOCKET) return NULL;		
	kn_socket *s = (kn_socket*)h;
	return &s->addr_local;
}

kn_sockaddr* kn_sock_addrpeer(handle_t h){
	if(((handle_t)h)->type != KN_SOCKET) return NULL;		
	kn_socket *s = (kn_socket*)h;
	return &s->addr_remote;	
}

engine_t kn_sock_engine(handle_t h){
	if(((handle_t)h)->type != KN_SOCKET) return NULL;
	kn_socket *s = (kn_socket*)h;
	return s->e;	
}

int      kn_close_sock(handle_t h){
	if(h->type != KN_SOCKET) 
		return -1;
	if(((kn_socket*)h)->type == SOCK_STREAM)
		return stream_socket_close(h);
	else
		return -1;
}
