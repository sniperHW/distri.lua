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
#include "linklist.h"
#include <stdio.h>

SHM_LNKLIST(linklist_int,int,16);

int main(){
	linklist_int *l = NULL;
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
	l = (linklist_int*)mmap(NULL,stat.st_size,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
	int i = 0;
	for(; i < 16; ++i){
		int ret;
		if(linklist_int_pop(l,&ret) == 0){
			printf("%d\n",ret);
		}
	}
	return 0;
}