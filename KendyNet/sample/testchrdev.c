#include "kn_chr_dev.h"
#include "kendynet.h"


handle_t sock = NULL;


st_io *new_stio(int size){
		st_io * io = calloc(1,sizeof(*io));
		struct iovec *wbuf = calloc(1,sizeof(*wbuf));
		char   *buf = calloc(1,size);
		wbuf->iov_base = buf;
		wbuf->iov_len = size;	
		io->iovec_count = 1;
		io->iovec = wbuf;
		return io;	
}

void release_stio(void *_){
	st_io *io = ((st_io*)_);
	free(io->iovec->iov_base);
	free(io->iovec);
	free(io);
}

void sock_transfer_finish(handle_t s,void *_,int32_t bytestransfer,int32_t err){
    	st_io *io = ((st_io*)_); 
	if(!io || bytestransfer <= 0)
	{
		kn_close_sock(s);
		sock = NULL;         
		if(!io) return;
	}
	if((uint64_t)io->ud == 2){
		printf("%s",(char*)io->iovec->iov_base);
	}
	release_stio(io);
}
	
void on_connect(handle_t s,void *_1,int _2,int err)
{
	if(err == 0){
		sock = s;
		printf("connect ok\n");
		kn_sock_set_clearfunc(s,release_stio);
		engine_t p = kn_sock_engine(s);
		kn_engine_associate(p,s,sock_transfer_finish);
	}else{
		printf("connect failed\n");
	}	
}


void chr_transfer_finish(handle_t s,void *_,int32_t bytestransfer,int32_t err){
    	st_io *io = ((st_io*)_); 
	if(bytestransfer <= 0)
	{       
		kn_release_chrdev(s,1);
	}else{
		((char*)io->iovec[0].iov_base)[bytestransfer] = 0;
		if(sock){
			st_io *io_send = new_stio(bytestransfer+1);
			strcpy(io_send->iovec->iov_base,((char*)io->iovec[0].iov_base));
			io_send->ud = (void*)1;
			kn_sock_send(sock,io_send);
			st_io *io_recv = new_stio(bytestransfer+1);
			io_recv->ud = (void*)2;
			kn_sock_recv(sock,io_recv);
		}	
		kn_chrdev_read(s,io);
	}
}


int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);
	engine_t p = kn_new_engine();
	handle_t h = kn_new_chrdev(0);
	if(!h) return 0;
	kn_engine_associate(p,h,chr_transfer_finish);
	st_io io;
	io.next.next = NULL;
	struct iovec wbuf[1];
	char   buf[4096];
	wbuf[0].iov_base = buf;
	wbuf[0].iov_len = 4096;	
	io.iovec_count = 1;
	io.iovec = wbuf;
	kn_chrdev_read(h,&io);
	kn_sockaddr remote;
	kn_addr_init_in(&remote,argv[1],atoi(argv[2]));
	handle_t c = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	int ret = kn_sock_connect(c,&remote,NULL);
	if(ret > 0){
		sock = c;
		printf("connect ok\n");
		kn_sock_set_clearfunc(c,release_stio);
		kn_engine_associate(p,c,sock_transfer_finish);		
	}else if(ret == 0){
		kn_engine_associate(p,c,on_connect);
	}else{
		kn_close_sock(c);
		printf("connect failed\n");
	}	
	kn_engine_run(p);	
	return 0;
}