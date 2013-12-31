/*	
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/	
#ifndef _EXCEPTION_H
#define _EXCEPTION_H


#include <stdlib.h>

#define MAX_EXCEPTION 4096
extern const char* exceptions[MAX_EXCEPTION];
#define except_alloc_failed     1   //内存分配失败
#define except_list_empty       2   //list_pop操作,当list为空触发
#define segmentation_fault      3
#define testexception1          4
#define testexception2          5
#define testexception3          6
//............


static inline const char *exception_description(int expno)
{
    if(expno >= MAX_EXCEPTION) return "unknow exception";
    if(exceptions[expno] == NULL) return "unknow exception";
    return exceptions[expno];
}


#endif
