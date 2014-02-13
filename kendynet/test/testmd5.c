#include <stdio.h>
#include "core/md5.h"
#include <stdint.h>

int main()
{
	char buf[32];
	printf("%s\n",md5str("12345",5,buf,32));
	return 0;
}
