#include "kn_chr_dev.h"
#include "kendynet.h"

st_io net_io;
struct iovec net_wbuf[1];
char   net_buf[4096];
void sock_transfer_finish(handle_t s,st_io *io,int32_t bytestransfer,int32_t err){


}
	
void on_connect(handle_t s,int err,void *ud)
{
	if(s){
		printf("connect ok\n");
		net_io.next.next = NULL;
		/*struct session *session = calloc(1,sizeof(*session));
		session->s = s;
		engine_t p = (engine_t)ud;
		kn_sock_associate(s,p,transfer_finish,NULL);	
		kn_sock_setud(s,session);    	
		session_send(session,send_size);*/
	}else{
		printf("connect failed\n");
	}	
}



void chr_transfer_finish(handle_t s,st_io *io,int32_t bytestransfer,int32_t err){
	if(bytestransfer <= 0)
	{       
		printf("read EOF\n");
		kn_release_chrdev(s,1);
	}else{
		((char*)io->iovec[0].iov_base)[bytestransfer] = 0;
		printf("%s\n",(char*)io->iovec[0].iov_base);
		kn_chrdev_read(s,io);
	}
}


int main(){
	engine_t p = kn_new_engine();
	handle_t h = kn_new_chrdev(0);
	if(!h) return 0;
	kn_chrdev_associate(h,p,chr_transfer_finish,NULL);
	st_io io;
	io.next.next = NULL;
	struct iovec wbuf[1];
	char   buf[4096];
	wbuf[0].iov_base = buf;
	wbuf[0].iov_len = 4096;	
	io.iovec_count = 1;
	io.iovec = wbuf;
	kn_chrdev_read(h,&io);
	kn_engine_run(p);	
	return 0;
}
