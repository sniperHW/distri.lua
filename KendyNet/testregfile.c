#include "kn_reg_file.h"


void transfer_finish(handle_t s,st_io *io,int32_t bytestransfer,int32_t err){
	if(bytestransfer <= 0)
	{       
		printf("read EOF\n");
		kn_release_regfile(s,1);
	}else{
		printf("%s\n",(char*)io->iovec[0].iov_base);
		kn_regfile_read(s,io);
	}
}


int main(){
	engine_t p = kn_new_engine();
	int fd = open("./README.md",O_RDONLY);
	handle_t h = kn_new_regfile(fd);
	if(!h) return 0;
	kn_regfile_associate(h,p,transfer_finish,NULL);
	st_io io;
	io.next.next = NULL;
	struct iovec wbuf[1];
	char   buf[4096];
	wbuf[0].iov_base = buf;
	wbuf[0].iov_len = 4096;	
	io.iovec_count = 1;
	io.iovec = wbuf;
	kn_regfile_read(h,&io);
	kn_engine_run(p);	
	return 0;
}
