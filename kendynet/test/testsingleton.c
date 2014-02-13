#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>  
#include "core/singleton.h"


typedef struct testst{
    int32_t value;
}testst;

testst *create_test(){
    printf("create_test\n");
    testst *t = calloc(1,sizeof(*t));
    t->value = 100;
    return t;
}

void destroy_test(void *ud){
	printf("destroy_test\n");
	free(ud);
}

DECLARE_SINGLETON(testst);
IMPLEMENT_SINGLETON(testst,create_test,destroy_test);


int main()
{
    testst *t = GET_INSTANCE(testst);
    printf("%d\n",t->value);
    GET_INSTANCE(testst);
    return 0;
}    