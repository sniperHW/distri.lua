#ifndef _MY_TOP_H
#define _MY_TOP_H

#include "top.h"
#include <stdio.h>
//非线程安全函数，禁止多线程访问

void addfilter(const char *);
const char*top();

#endif
