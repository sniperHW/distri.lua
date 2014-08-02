#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "log.h"
#include "kn_time.h"
#include "kendynet.h"

DEF_LOG(testlog,"testlog");
IMP_LOG(testlog);

#define LOG_TEST(LOGLEV,...) LOG(GET_LOGFILE(testlog),LOGLEV,__VA_ARGS__)
int main(int argc,char **argv)
{
    SYS_LOG(LOG_INFO,"begin");
    uint32_t i = 0;
    for(i=0; i < 100; ++i)
        LOG_TEST(LOG_INFO,"haha");
    printf("end\n");
    return 0;
}
