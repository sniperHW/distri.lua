#include <stdio.h>
#include <stdlib.h>
#include "core/systime.h"


int main()
{
    {
        uint32_t begin = GetSystemMs();
        int i = 0;
        for(; i < 100000000;++i)
            GetSystemSec();
        printf("100000000 call GetSystemSec use %u ms\n",GetSystemMs()-begin);
    }

    {
        uint32_t begin = GetSystemMs();
        int i = 0;
        for(; i < 100000000;++i)
            time(NULL);
        printf("100000000 call time(NULL) use %u ms\n",GetSystemMs()-begin);
    }
    return 0;
}
