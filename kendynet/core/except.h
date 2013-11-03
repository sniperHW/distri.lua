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
#ifndef _EXCEPT_H
#define _EXCEPT_H
#include <setjmp.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

struct exception_frame
{
	struct exception_frame *pre;
	jmp_buf jumpbuffer;
	int32_t exception;
	int32_t line;
	const char *file;
};

extern struct exception_frame *exception_stack;

extern void exception_throw(int32_t code,const char *file,int32_t line);

/*一个函数第一次使用TRY前必须定义,如果重复定义会出现不确定的行为
* 主要是为了正确处理在TRY代码内的RETURN
*/
#define FUNCTION_TRY int32_t function_except_stack_count = 0; 
#define TRY do{\
	++function_except_stack_count;\
	struct exception_frame  frame;\
	frame.pre = exception_stack;\
	exception_stack = &frame;\
	if((frame.exception = setjmp(frame.jumpbuffer)) == 0)
	
#define THROW(EXP) exception_throw(EXP,__FILE__,__LINE__)

#define RETHROW exception_throw(frame.exception,frame.file,frame.line)

#define CATCH(EXP) else if(EXP == frame.exception ? frame.exception = 0,1:0)

#define CATCH_ALL else if(frame.exception ? frame.exception = 0,1:0)

#define ENDTRY --function_except_stack_count;\
			   if(frame.exception){\
					exception_throw(frame.exception,frame.file,frame.line);}\
			   else if(exception_stack){\
					exception_stack = exception_stack->pre;\
					}\
			}while(0);					

#define FINALLY
/*根据当前函数中try的处理情况丢弃数量正确的异常栈,再返回*/
#define RETURN(R) do{if(exception_stack){\
						while(exception_stack && function_except_stack_count>0){\
							function_except_stack_count--;\
							exception_stack = exception_stack->pre;\
						}\
					 }\
			         return R;\
				  }while(0)

#endif