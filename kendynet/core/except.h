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
#include <string.h>
#include "llist.h"


struct callstack_frame
{
    lnode node;
    char  info[256];
};

struct exception_frame
{
    lnode   node;
	sigjmp_buf jumpbuffer;
	int32_t exception;
	int32_t line; 
    const char   *file;
    const char   *func;
    int8_t  is_process;
    struct llist  call_stack; //函数调用栈记录 
	
};

struct exception_perthd_st
{
	struct llist  expstack;
	struct llist  csf_pool;   //callstack_frame池
};

extern pthread_key_t g_exception_key;
extern pthread_once_t g_exception_key_once;


void exception_once_routine();

static inline void clear_call_stack(struct exception_frame *frame)
{
    struct exception_perthd_st *epst = (struct exception_perthd_st*)pthread_getspecific(g_exception_key);
	while(!LLIST_IS_EMPTY(&frame->call_stack))
        LLIST_PUSH_FRONT(&epst->csf_pool,LLIST_POP(lnode*,&frame->call_stack));
}

static inline void print_call_stack(struct exception_frame *frame)
{
    if(!frame)return;
    struct lnode *node = llist_head(&frame->call_stack);
    int f = 0;
    while(node != NULL)
    {
        struct callstack_frame *cf = (struct callstack_frame*)node;
        printf("% 2d: %s",++f,cf->info);
        node = node->next;
    }
}

#define PRINT_CALL_STACK print_call_stack(&frame)


static inline struct llist *GetCurrentThdExceptionStack()
{
    pthread_once(&g_exception_key_once,exception_once_routine);
	struct exception_perthd_st *epst = (struct exception_perthd_st *)pthread_getspecific(g_exception_key);
	if(!epst)
	{
		epst = calloc(1,sizeof(*epst));
		int32_t i = 0;
		for( ; i < 256; ++i){
			struct callstack_frame *call_frame = calloc(1,sizeof(*call_frame));
			LLIST_PUSH_FRONT(&epst->csf_pool,(lnode*)call_frame);
		}
        pthread_setspecific(g_exception_key,epst);
	}
	return &epst->expstack;
}

static inline void expstack_push(struct exception_frame *frame)
{
    struct llist *expstack = GetCurrentThdExceptionStack();
// #ifdef _DEBUG
//	printf("push %s,%s\n",frame->file,frame->func);
// #endif
	LLIST_PUSH_FRONT(expstack,(lnode*)frame);
}

static inline struct exception_frame* expstack_pop()
{
    struct llist *expstack = GetCurrentThdExceptionStack();
    struct exception_frame *frame = LLIST_POP(struct exception_frame*,expstack);
//#ifdef _DEBUG
//    if(frame) printf("pop %s,%s\n",frame->file,frame->func);
//#endif
    return frame;
}

static inline struct exception_frame* expstack_top()
{
    struct llist *expstack = GetCurrentThdExceptionStack();
    return (struct exception_frame*)llist_head(expstack);
}

extern void exception_throw(int32_t code,const char *file,const char *func,int32_t line);

#define TRY do{\
	struct exception_frame  frame;\
    frame.node.next = NULL;\
    frame.file = __FILE__;\
    frame.func = __FUNCTION__;\
    frame.exception = 0;\
    frame.is_process = 1;\
    llist_init(&frame.call_stack);\
    expstack_push(&frame);\
    int savesigs= SIGSEGV | SIGBUS | SIGFPE;\
	if(sigsetjmp(frame.jumpbuffer,savesigs) == 0)
	
#define THROW(EXP) exception_throw(EXP,__FILE__,__FUNCTION__,__LINE__)

#define CATCH(EXP) else if(!frame.is_process && frame.exception == EXP?\
                        frame.is_process=1,frame.is_process:frame.is_process)

#define CATCH_ALL else if(!frame.is_process?\
                        frame.is_process=1,frame.is_process:frame.is_process)

#define ENDTRY if(!frame.is_process)\
                    exception_throw(frame.exception,frame.file,frame.func,frame.line);\
               else {\
                    struct exception_frame *frame = expstack_pop();\
                    clear_call_stack(frame);\
                }\
			}while(0);					

//#define FINALLY
/*根据当前函数中try的处理情况丢弃数量正确的异常栈,再返回*/
/*#define RETURN  do{struct exception_frame *top;\
                    while((top = expstack_top())!=NULL){\
                        if(top->file == __FILE__ && top->func == __FUNCTION__)\
                        {\
                            struct exception_frame *frame = expstack_pop();\
                            clear_call_stack(frame);\
                        }else\
                        break;\
                    };\
                }while(0);return
*/

#define RETURN  do{struct exception_frame *top;\
                    while((top = expstack_top())!=NULL){\
                        if(strcmp(top->file,__FILE__) == 0 && strcmp(top->func,__FUNCTION__) == 0)\
                        {\
                            struct exception_frame *frame = expstack_pop();\
                            clear_call_stack(frame);\
                        }else\
                        break;\
                    };\
                }while(0);return			

#define EXPNO frame.exception


#endif
