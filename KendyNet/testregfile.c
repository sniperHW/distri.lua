#include "kn_reg_file.h"


void transfer_finish(handle_t s,st_io *io,int32_t bytestransfer,int32_t err){
	if(bytestransfer <= 0)
	{       
		printf("read EOF\n");
		free(io->iovec[0].iov_base);
		free(io);
		kn_release_regfile(s,1);
		break;
	}else{
		printf("%s\n",io->iovec[0].iov_base);
		kn_regfile_read(s,io);
	}
}

void destry_stio(st_io *io){
	free(io->iovec[0].iov_base);
	free(io);	
}

int main(){
	engine_t p = kn_new_engine();
	int fd = open("./README.md",O_RDONLY);
	handle_t h = kn_new_regfile(fd);
	if(!h) return 0;
	kn_regfile_associate(h,p,transfer_finish,destry_stio);
	st_io *io = calloc(1,sizeof(*io));
	io->iovec[0].iov_base = calloc(1,4096);
	io->iovec[0].iov_len = 4096;
	kn_regfile_read(h,io);
	kn_engine_run(p);	
	return 0;
}
