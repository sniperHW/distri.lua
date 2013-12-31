#include <stdio.h>
#include "core/except.h"
#include "core/exception.h"
int testfunc1()
{
    TRY{
        TRY{
            RETURN 0;
        }
        ENDTRY;
    }
    ENDTRY;
    return 0;
}


void testfunc2()
{
    THROW(testexception1);
}

void testfunc3()
{
    THROW(testexception2);
}

int main()
{
    TRY{
        testfunc1();
        printf("after testfunc1\n");
    }
    ENDTRY;

    TRY{
        testfunc2();
        printf("after testfunc1\n");
    }CATCH(testexception1)
    {
       printf("catch:%s\n",exception_description(EXPNO));
    }
    ENDTRY;

    TRY{
        testfunc3();
        printf("after testfunc3\n");
    }CATCH(testexception1)
    {
        printf("catch:%s\n",exception_description(EXPNO));
    }CATCH_ALL
    {
        printf("catch all: %s\n",exception_description(EXPNO));
    }
    ENDTRY;

    return 0;
}
