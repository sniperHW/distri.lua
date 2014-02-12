#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "core/log.h"
#include "core/systime.h"

int main(int argc,char **argv)
{
    uint32_t i = 0;
    logfile_t testlog = create_logfile("testlog");
    for(i=0; i < 1000000; ++i)
        LOG(testlog,LOG_INFO,"haha");
    return 0;
}