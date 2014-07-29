#include <sys/types.h>
#include <sys/stat.h>
#include "kn_type.h"
#include "kn_reg_file.h"


typedef struct{
	handle comm_head;
	int    events;
	int    processing;
	engine_t e;
	kn_list pending_write;//尚未处理的发请求
    kn_list pending_read;//尚未处理的读请求
	void   (*cb_ontranfnish)(handle_t,st_io*,int,int);
	void   (*destry_stio)(st_io*);
}kn_reg_file;


enum{
	KN_REGFILE_RELEASE = 1,
};


static void process_read(kn_reg_file *r){
	st_io* io_req = 0;
	int bytes_transfer = 0;
	int total_transfer = 0;
	while(total_transfer < 65536 && (io_req = (st_io*)kn_list_pop(&s->pending_read))!=NULL){
		errno = 0;
		bytes_transfer = TEMP_FAILURE_RETRY(readv(s->comm_head.fd,io_req->iovec,io_req->iovec_count));
		if(bytes_transfer < 0 && errno == EAGAIN){
				//将请求重新放回到队列
				kn_list_pushback(&s->pending_read,(kn_list_node*)io_req);
				break;
		}else{
			r->cb_ontranfnish((handle_t)r,io_req,bytes_transfer,errno);
			if(r->comm_head.status == KN_REGFILE_RELEASE)
				return;			
		}
	}	
	if(kn_list_size(&s->pending_read) == 0){
		//没有接收请求了,取消EPOLLIN
		int events = s->events ^ EPOLLIN;
		if(0 == kn_event_mod(r->e,(handle_t)r,events))
			r->events = events;
	}
}

static void process_write(kn_reg_file *s){
	st_io* io_req = 0;
	int bytes_transfer = 0;
	int total_transfer = 0;
	while(total_transfer < 65536 && (io_req = (st_io*)kn_list_pop(&r->pending_write))!=NULL){
		errno = 0;
		bytes_transfer = TEMP_FAILURE_RETRY(writev(r->comm_head.fd,io_req->iovec,io_req->iovec_count));
		if(bytes_transfer < 0 && errno == EAGAIN){
				//将请求重新放回到队列
				kn_list_pushback(&r->pending_write,(kn_list_node*)io_req);
				break;
		}else{
			r->cb_ontranfnish((handle_t)r,io_req,bytes_transfer,errno);
			if(r->comm_head.status == KN_REGFILE_RELEASE)
				return;
		}
	}
	if(kn_list_size(&r->pending_write) == 0){
		//没有接收请求了,取消EPOLLOUT
		int events = r->events ^ EPOLLOUT;
		if(0 == kn_event_mod(r->e,(handle_t)r,events))
			r->events = events;
	}		
}


static void destroy_regfile(kn_reg_file *r){
	printf("destroy_regfile\n");
	st_io *io_req;
	if(r->destry_stio){
        while((io_req = (st_io*)kn_list_pop(&s->pending_write))!=NULL)
            r->destry_stio(io_req);
        while((io_req = (st_io*)kn_list_pop(&s->pending_read))!=NULL)
            r->destry_stio(io_req);
	}
	free(r);
}


static void on_events(handle_t h,int events){
	kn_reg_file *r = (kn_reg_file*)h;
	r->processing = 1;
	do{
		
		if(events & EPOLLIN){
			process_read(r);
			if(r->comm_head.status == KN_REGFILE_RELEASE)
				break;
		}		
		if(events & EPOLLOUT){
			process_write(r);
		}
	}while(0);
	r->processing = 0;
	if(r->comm_head.status == KN_REGFILE_RELEASE){
		destroy_regfile(r);
	}	
}

handle_t kn_new_regfile(int fd){
	struct stat buf;
	if(0 != fstat(fd,&buf)) return NULL;
	if(!S_ISREG(buf.st_mode)) return NULL; 
	kn_reg_file *r = calloc(1,sizeof(*r));
	((handle_t)r)->fd = fd;
	((handle_t)r)->type = KN_REGFILE;
	return r;
}

//如果close非0,则同时调用close(fd)
int kn_release_regfile(handle_t h,int beclose){
	if(((handle)h)->type != KN_REGFILE) return -1;
	if(((handle)h)->status == KN_REGFILE_RELEASE) return -1;
	kn_reg_file *r = (kn_reg_file*)h;
	if(beclose) close(r->comm_head.fd);
	if(r->processing){
		r->comm_head.status = KN_REGFILE_RELEASE;
	}else{
			//可以安全释放
		destroy_regfile(r);
	}	
	return 0;	
}

int kn_regfile_fd(handle_t h){
	return ((handle)h)->fd; 
}

int kn_regfile_associate(handle_t h,
						 engine_t e,
						 void (*cb_ontranfnish)(handle_t,st_io*,int,int),
						 void (*destry_stio)(st_io*))
{
	if(((handle)h)->type != KN_REGFILE) return -1;
	if(((handle)h)->status == KN_REGFILE_RELEASE) return -1;
	kn_reg_file *r = (kn_reg_file*)h;	
	if(!cb_ontranfnish) return -1;
	if(r->e) kn_event_del(r->e,h);
	r->destry_stio = destry_stio;
	r->cb_ontranfnish = cb_ontranfnish;
	r->e = e;		
	return 0;
}
						   
						   
int kn_regfile_write(handle_t h,st_io *req){
	if(((handle)h)->type != KN_REGFILE) return -1;
	if(((handle)h)->status == KN_REGFILE_RELEASE) return -1;
	kn_reg_file *r = (kn_reg_file*)h;	
	kn_list_pushback(&r->pending_write,(kn_list_node*)req);
	if(!(r->events & EPOLLOUT)){
		int ret = 0;
		if(r->events == 0){
			ret = kn_event_add(r->e,(handle_t)r,EPOLLOUT);
		}else
			ret = kn_event_mod(r->e,(handle_t)r,EPOLLOUT);
			
		if(ret == 0)
			r->events = EPOLLOUT;
		else
			return -1;
	}		
	return 0;
}

int kn_regfile_read(handle_t h,st_io *req){
	if(((handle)h)->type != KN_REGFILE) return -1;
	if(((handle)h)->status == KN_REGFILE_RELEASE) return -1;
	kn_reg_file *r = (kn_reg_file*)h;	
	kn_list_pushback(&r->pending_read,(kn_list_node*)req);
	if(!(r->events & EPOLLIN)){
		int ret = 0;
		if(r->events == 0){
			ret = kn_event_add(r->e,(handle_t)r,EPOLLIN);
		}else
			ret = kn_event_mod(r->e,(handle_t)r,EPOLLIN);
			
		if(ret == 0)
			r->events = EPOLLIN;
		else
			return -1;
	}		
	return 0;
}	
