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
#include             <stdint.h>
#include            "lockfree/kn_ringque.h"

DECLARE_RINGQUE_1(ring1_int,int,1024,1);

int main()
{

	ring1_int *ringque = NULL;
	int c,fd,flags;
	char *ptr;
	off_t length;
	flags = O_RDWR | O_CREAT;
	length = sizeof(*ringque);
	fd = shm_open("kenny",flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(fd < 0){
			printf("error\n");
			exit(0);
	}		
	ftruncate(fd,length);
	ringque = (ring1_int*)mmap(NULL,length,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
	printf("%x\n",ringque);
	ring1_int_init(ringque);
	while(1){
		ring1_int_push(ringque,1);
	}
	return 0;
}
