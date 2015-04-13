#include <sys/types.h>
#include <sys/stat.h>
#include "kn_type.h"
#include "kn_chr_dev.h"
#include "kendynet_private.h"
#include "kn_event.h"

typedef struct{
	handle comm_head;
	engine_t e;
	kn_list pending_write;//尚未处理的发请求
    	kn_list pending_read;//尚未处理的读请求
	void   (*callback)(handle_t,void*,int,int);
	void   (*clear_func)(void*);
}kn_chr_dev;


enum{
	KN_CHRDEV_RELEASE = 1,
};

static void process_read(kn_chr_dev *r){
	st_io* io_req = 0;
	int bytes_transfer = 0;
	while((io_req = (st_io*)kn_list_pop(&r->pending_read))!=NULL){
		errno = 0;
		bytes_transfer = TEMP_FAILURE_RETRY(readv(r->comm_head.fd,io_req->iovec,io_req->iovec_count));
		if(bytes_transfer < 0 && errno == EAGAIN){
				//将请求重新放回到队列
				kn_list_pushback(&r->pending_read,(kn_list_node*)io_req);
				break;
		}else{
			r->callback((handle_t)r,io_req,bytes_transfer,errno);
			if(r->comm_head.status == KN_CHRDEV_RELEASE)
				return;			
		}
	}	
	if(kn_list_size(&r->pending_read) == 0){
		//没有接收请求了,取消EPOLLIN
		kn_disable_read(r->e,(handle_t)r);
	}
}

static void process_write(kn_chr_dev *r){
	st_io* io_req = 0;
	int bytes_transfer = 0;
	while((io_req = (st_io*)kn_list_pop(&r->pending_write))!=NULL){
		errno = 0;
		bytes_transfer = TEMP_FAILURE_RETRY(writev(r->comm_head.fd,io_req->iovec,io_req->iovec_count));
		if(bytes_transfer < 0 && errno == EAGAIN){
				//将请求重新放回到队列
				kn_list_pushback(&r->pending_write,(kn_list_node*)io_req);
				break;
		}else{
			r->callback((handle_t)r,io_req,bytes_transfer,errno);
			if(r->comm_head.status == KN_CHRDEV_RELEASE)
				return;
		}
	}
	if(kn_list_size(&r->pending_write) == 0){
		//没有接收请求了,取消EPOLLOUT
		kn_disable_write(r->e,(handle_t)r);		
	}		
}


static void on_destroy(void *_){
	kn_chr_dev *r = (kn_chr_dev*)_;
	st_io *io_req;
	if(r->clear_func){
        		while((io_req = (st_io*)kn_list_pop(&r->pending_write))!=NULL)
            			r->clear_func(io_req);
        		while((io_req = (st_io*)kn_list_pop(&r->pending_read))!=NULL)
            			r->clear_func(io_req);
	}
	kn_event_del(r->e,(handle_t)r);
	free(r);	
}

static void on_events(handle_t h,int events){
	kn_chr_dev *r = (kn_chr_dev*)h;
	do{
		h->inloop = 1;
		if(events & EVENT_READ){
			process_read(r);
			if(h->status == KN_CHRDEV_RELEASE)
				break;
		}		
		if(events & EVENT_WRITE){
			process_write(r);
		}
		h->inloop = 0;
	}while(0);
	if(h->status == KN_CHRDEV_RELEASE)
		on_destroy(r);	
}

static int chrdev_associate(engine_t e,
		                   handle_t h,
		                   void (*callback)(handle_t,void*,int,int))
{
	if(((handle_t)h)->type != KN_CHRDEV) return -1;
	if(((handle_t)h)->status == KN_CHRDEV_RELEASE) return -1;
	kn_chr_dev *r = (kn_chr_dev*)h;	
	if(r->e) kn_event_del(r->e,h);
	r->callback = callback;
	r->e = e;
#ifdef _LINUX
	kn_event_add(r->e,h,EPOLLERR);
#elif   _BSD
	kn_event_add(r->e,h,EVFILT_READ);
	kn_event_add(r->e,h,EVFILT_WRITE);
	kn_disable_read(r->e,h);
	kn_disable_write(r->e,h);
#endif			
	return 0;
}

handle_t kn_new_chrdev(int fd){
	struct stat buf;
	if(0 != fstat(fd,&buf)) return NULL;
	if(!S_ISCHR(buf.st_mode)) return NULL; 
	kn_chr_dev *r = calloc(1,sizeof(*r));
	((handle_t)r)->fd = fd;
	kn_set_noblock(fd,0);
	((handle_t)r)->type = KN_CHRDEV;
	((handle_t)r)->on_events = on_events;
	//((handle_t)r)->on_destroy = on_destroy;
	((handle_t)r)->associate = chrdev_associate;
	return (handle_t)r;
}


//如果close非0,则同时调用close(fd)
int kn_release_chrdev(handle_t h,int beclose){
	if(((handle_t)h)->type != KN_CHRDEV) return -1;
	if(((handle_t)h)->status == KN_CHRDEV_RELEASE) return -1;
	kn_chr_dev *r = (kn_chr_dev*)h;
	if(beclose) close(r->comm_head.fd);
	if(h->inloop){
		r->comm_head.status = KN_CHRDEV_RELEASE;
		//kn_push_destroy(r->e,(handle_t)r);
	}else
		on_destroy(r);
	return 0;	
}

int kn_chrdev_fd(handle_t h){
	return ((handle_t)h)->fd; 
}
						  						   
int kn_chr_dev_write(handle_t h,st_io *req){
	if(((handle_t)h)->type != KN_CHRDEV) return -1;
	if(((handle_t)h)->status == KN_CHRDEV_RELEASE) return -1;
	kn_chr_dev *r = (kn_chr_dev*)h;	
	 if(!is_set_write((handle_t)r)){
	 	if(0 != kn_enable_write(r->e,(handle_t)r))
	 		return -1;
	 }		
	kn_list_pushback(&r->pending_write,(kn_list_node*)req);			
	return 0;
}

int kn_chrdev_read(handle_t h,st_io *req){
	if(((handle_t)h)->type != KN_CHRDEV) return -1;
	if(((handle_t)h)->status == KN_CHRDEV_RELEASE) return -1;
	kn_chr_dev *r = (kn_chr_dev*)h;	
	 if(!is_set_read((handle_t)r)){
	 	if(0 != kn_enable_read(r->e,(handle_t)r))
	 		return -1;
	 }	
	kn_list_pushback(&r->pending_read,(kn_list_node*)req);			
	return 0;
}

void   kn_chrdev_set_clear_func(handle_t h,void (*func)(void*)){
	if(((handle_t)h)->type == KN_CHRDEV)
		((kn_chr_dev*)h)->clear_func = func; 
}
