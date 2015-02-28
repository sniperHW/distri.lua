#include 	<sys/mman.h>
#include	<sys/types.h>	/* basic system data types */
#include	<sys/time.h>	/* timeval{} for select() */
#include	<time.h>		/* timespec{} for pselect() */
#include	<errno.h>
#include	<fcntl.h>		/* for nonblocking */
#include	<limits.h>		/* PIPE_BUF */
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>	/* for S_xxx file mode constants */
#include	<unistd.h>
#include	<sys/wait.h>
#include            "lockfree/kn_ringque.h"
#include            "kn_time.h"

DECLARE_RINGQUE_1(ring1_int,int,1024,1);

int main()
{
	int fd,flags;
	off_t length;
	flags = O_RDWR;
	fd = shm_open("kenny",flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(fd < 0)
	{
		printf("error\n");
		exit(0);
	}
	struct stat stat;
	fstat(fd,&stat);
	ring1_int *ringque = NULL;	
	ringque = (ring1_int*)mmap(NULL,stat.st_size,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);	
	printf("%x\n",ringque);

	uint64_t c = 0;
	uint64_t tick = kn_systemms64();
	while(1){
		int msg;
		if(0 == ring1_int_pop(ringque,&msg)){
			++c;
		}
		uint64_t now = kn_systemms64();
		if(now - tick >= 1000){
			uint64_t elapse = now - tick;
			printf("%lu\n",(c * 1000)/elapse);
			tick = now;
			c = 0;
		}
	}
	return 0;
}
