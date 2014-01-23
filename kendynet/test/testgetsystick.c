#include <stdio.h>
#include <stdlib.h>
#include "core/systime.h"


int main()
{
    {
        uint64_t begin = GetSystemMs();
        int i = 0;
        for(; i < 100000000;++i)
            GetSystemMs();
        printf("100000000 call GetSystemMs use %lld ms\n",GetSystemMs()-begin);
    }

    {
        uint64_t begin = GetSystemMs();
        int i = 0;
        for(; i < 100000000;++i)
            GetSystemSec();
        printf("100000000 call GetSystemSec() use %lld ms\n",GetSystemMs()-begin);
    }
    return 0;
}
