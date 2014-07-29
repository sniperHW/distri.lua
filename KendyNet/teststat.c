#include <sys/types.h>
#include <sys/stat.h>
#include "kendynet.h"



int main(){

	struct stat buf,buf1,buf2;
	int ret = fstat(0,&buf);
	if(S_ISCHR(buf.st_mode)) printf("is character dev\n");
	handle_t l = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	ret = fstat(kn_sock_fd(l),&buf1);
	if(S_ISSOCK(buf1.st_mode)) printf("is socket\n");
	int fd = open("./README.md",O_RDONLY);
	ret = fstat(fd,&buf2);
	if(S_ISREG(buf2.st_mode)) printf("is reg file\n");
	printf("%d\n",fd);
	return 0;
}
