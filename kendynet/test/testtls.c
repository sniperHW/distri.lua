#include <stdio.h>
#include "core/tls.h"

void test_fn(void *ud)
{
    printf("%d\n",*(int*)ud);
}

int main()
{

    int data = 10;
    tls_create(0,&data,test_fn);
    int *tmp = (int*)tls_get(0);
    printf("%d\n",*tmp);
    pthread_exit(NULL);
    return 0;
}
