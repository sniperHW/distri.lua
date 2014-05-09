#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "log.h"
#include "kn_time.h"


DEF_LOG(testlog,"testlog");

#define LOG_TEST(LOGLEV,...) LOG(GET_LOGFILE(testlog),LOGLEV,__VA_ARGS__)

int main(int argc,char **argv)
{
    uint32_t i = 0;
    for(i=0; i < 1000000; ++i)
        LOG_TEST(LOG_INFO,"haha");
    return 0;
}
