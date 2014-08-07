#include "kn_datasocket.h"
#include "kn_proactor.h"

//未连接的数据报协议
#define PREPARE_MSG(CONNECTED,MSG,FAMILY,REQ)\
	struct msghdr  MSG = {0};\
	if(!CONNECTED){\
		if(FAMILY == AF_INET){ \
			MSG.msg_namelen = sizeof(REQ->addr.in);\
			MSG.msg_name = &REQ->addr.in;\
		}else if(FAMILY == AF_INET6){ \
			MSG.msg_namelen = sizeof(REQ->addr.in6);\
			MSG.msg_name = &REQ->addr.in6;\
		}else if(FAMILY == AF_LOCAL){ \
			MSG.msg_namelen = sizeof(REQ->addr.un);\
			MSG.msg_name = &REQ->addr.un;\
		}\
	}\
    MSG.msg_iov = REQ->iovec;\
    MSG.msg_iovlen = REQ->iovec_count

static int32_t kn_dgram_raw_recv(kn_datasocket* d,st_io *io_req,int32_t *err_code)
{
	int32_t        ret;
    int            family = d->addr_local.addrtype;
	PREPARE_MSG(d->connected,msg,family,io_req);
	*err_code = 0;
	ret  = TEMP_FAILURE_RETRY(recvmsg(d->base.fd,&msg,0));
	if(ret < 0)
	{
		*err_code = errno;
		if(*err_code == EAGAIN)
		{
			d->flag &= (~readable);
			//将请求重新放回到队列
            kn_list_pushback(&d->pending_recv,(kn_list_node*)io_req);
		}
	}
	return ret;
}

static int32_t kn_dgram_raw_send(kn_datasocket* d,st_io *io_req,int32_t *err_code)
{
	int32_t        ret;
    int            family = d->addr_local.addrtype;
	PREPARE_MSG(d->connected,msg,family,io_req);
	*err_code = 0;
	ret  = TEMP_FAILURE_RETRY(sendmsg(d->base.fd,&msg,0));
	if(ret < 0)
	{
		*err_code = errno;
		if(*err_code == EAGAIN)
		{
			d->flag &= (~writeable);
			//将请求重新放回到队列
            kn_list_pushback(&d->pending_send,(kn_list_node*)io_req);
		}
	}
	return ret;
}

//用于已连接的数据报协议或流协议
static int32_t kn_stream_raw_recv(kn_datasocket* d,st_io *io_req,int32_t *err_code)
{
	int32_t ret;
	*err_code = 0;
	ret  = TEMP_FAILURE_RETRY(readv(d->base.fd,io_req->iovec,io_req->iovec_count));
	if(ret < 0)
	{
		*err_code = errno;
		if(*err_code == EAGAIN)
		{
			d->flag &= (~readable);
			//将请求重新放回到队列
            kn_list_pushback(&d->pending_recv,(kn_list_node*)io_req);
		}
	}
	return ret;
}

static int32_t kn_stream_raw_send(kn_datasocket* d,st_io *io_req,int32_t *err_code)
{

	int32_t ret;
	*err_code = 0;
	ret  = TEMP_FAILURE_RETRY(writev(d->base.fd,io_req->iovec,io_req->iovec_count));
	if(ret < 0)
	{
		*err_code = errno;
		if(*err_code == EAGAIN)
		{
			d->flag &= (~writeable);
			//将请求重新放回到队列
            kn_list_pushback(&d->pending_send,(kn_list_node*)io_req);
		}
	}
	return ret;
}

void kn_datasocket_on_active(kn_fd_t s,int event){
	kn_datasocket* t = (kn_datasocket*)s;
	char buf[1];
	if(event & (EPOLLERR | EPOLLHUP)){
		kn_ref_acquire(&s->ref);//防止s在cb_transfer中被释放
		errno = 0;
		read(s->fd,buf,1);//触发errno变更
		t->cb_transfer(s,NULL,-1,errno);
		kn_ref_release(&s->ref);
		return;
	}
	if(event & (EPOLLRDHUP | EPOLLIN)){
		t->flag |= readable;
		if(kn_list_size(&t->pending_recv))
         	kn_procator_putin_active(s->proactor,s);
	}
	if(event & EPOLLOUT){
		t->flag |= writeable;
		if(kn_list_size(&t->pending_send))
         	kn_procator_putin_active(s->proactor,s);
	}
}

static inline void _recv(kn_datasocket* t)
{
	st_io* io_req = 0;
	int32_t err_code = 0;
	int32_t bytes_transfer;
	if(t->flag & readable)
	{
        if((io_req = (st_io*)kn_list_pop(&t->pending_recv))!=NULL)
		{
			bytes_transfer = t->raw_recv(t,io_req,&err_code);
			//if(bytes_transfer == 0) bytes_transfer = -1;
			if(err_code != EAGAIN)  t->cb_transfer((kn_fd_t)t,io_req,bytes_transfer,err_code);
		}
	}
}

static inline void _send(kn_datasocket* t)
{
	st_io* io_req = 0;
	int32_t err_code = 0;
	int32_t bytes_transfer;
	if(t->flag & writeable)
	{
        if((io_req = (st_io*)kn_list_pop(&t->pending_send))!=NULL)
		{
			bytes_transfer = t->raw_send(t,io_req,&err_code);
			//if(bytes_transfer == 0) bytes_transfer = -1;
			if(err_code != EAGAIN)  t->cb_transfer((kn_fd_t)t,io_req,bytes_transfer,err_code);
		}
	}	
}

int8_t  kn_datasocket_process(kn_fd_t s)
{
	kn_datasocket* t = (kn_datasocket *)s;
	kn_ref_acquire(&s->ref);//防止s在_recv/_send中回调cb_transfer时被释放
	_recv(t);
	_send(t);
    int32_t read_active = (t->flag & readable)  && kn_list_size(&t->pending_recv);
    int32_t write_active = (t->flag & writeable)&& kn_list_size(&t->pending_send);
	kn_ref_release(&s->ref);
	return (read_active || write_active);
}

void kn_datasocket_destroy(void *ptr){
	//printf("kn_datasocket_destroy,%x\n",(int)ptr);
	kn_datasocket *t = (kn_datasocket *)((char*)ptr - sizeof(kn_dlist_node));
	st_io *io_req;
	if(t->base.proactor) t->base.proactor->UnRegister(t->base.proactor,&t->base);	
	if(t->destroy_stio){
        while((io_req = (st_io*)kn_list_pop(&t->pending_send))!=NULL)
            t->destroy_stio(io_req);
        while((io_req = (st_io*)kn_list_pop(&t->pending_recv))!=NULL)
            t->destroy_stio(io_req);
	}
	close(t->base.fd);
	free(t);
}

kn_fd_t kn_new_datasocket(int fd,int sock_type,struct kn_sockaddr *local,struct kn_sockaddr *remote)
{
	kn_datasocket*    d = calloc(1,sizeof(*d));
	d->base.type = sock_type;
	d->base.fd = fd;
	d->base.on_active = kn_datasocket_on_active;
	d->base.process = kn_datasocket_process;
	if(local)  d->addr_local = *local;
	if(remote) d->addr_remote = *remote;
	if(sock_type == STREAM_SOCKET){
		d->connected = 1; 
		d->raw_send = kn_stream_raw_send;
		d->raw_recv = kn_stream_raw_recv;
	}else{
		if(remote) d->connected = 1; 
		d->raw_send = kn_dgram_raw_send;
		d->raw_recv = kn_dgram_raw_recv;
	}
	kn_ref_init(&d->base.ref,kn_datasocket_destroy);
	return (kn_fd_t)d;
}

