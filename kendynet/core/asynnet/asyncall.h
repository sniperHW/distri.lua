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
#ifndef _ASYNCALL_H
#define _ASYNCALL_H
/*
*  进程内的异步函数调用
*
*/

#include "kn_msg.h"
#include "msgdisp.h"

struct asyncall_context;
//异步调用返回后的回调函数
typedef void (*fn_asyncall_result)(struct asyncall_context*,void *result);

typedef struct asyncall_context{
	msgdisp_t          caller;	
	fn_asyncall_result fn_result;
	void*              result;
}asyncall_context,*asyncall_context_t;


//异步调用的函数指针
typedef void (*fn_asyncall)(asyncall_context_t,void **param);

typedef struct msg_asyncall{
	struct msg base;
	asyncall_context_t context;
	fn_asyncall        fn_call;
	void*              param[0];
}msg_asyncall,*msg_asyncall_t;

typedef struct msg_asynresult{
	struct msg base;
	asyncall_context_t context;
}msg_asynresult,*msg_asynresult_t;


//不关心返回值的时候sender,asyncall_context_t,fn_asyncall_result可以为NULL
int32_t asyncall(msgdisp_t sender,msgdisp_t recver,fn_asyncall,asyncall_context_t,fn_asyncall_result,...);

int32_t asynreturn(asyncall_context_t,void *result);

#define ASYNCALL0(SENDER,RECVER,FN_CALL,CONTEXT,FN_RESULT)\
		 asyncall(SENDER,RECVER,FN_CALL,(asyncall_context_t)CONTEXT,FN_RESULT,NULL)

#define ASYNCALL1(SENDER,RECVER,FN_CALL,CONTEXT,FN_RESULT,PARAM1)\
		 asyncall(SENDER,RECVER,FN_CALL,(asyncall_context_t)CONTEXT,FN_RESULT,(void*)PARAM1,NULL)

#define ASYNCALL2(SENDER,RECVER,FN_CALL,CONTEXT,FN_RESULT,PARAM1,PARAM2)\
		 asyncall(SENDER,RECVER,FN_CALL,(asyncall_context_t)CONTEXT,FN_RESULT,(void*)PARAM1,(void*)PARAM2,NULL)

#define ASYNCALL3(SENDER,RECVER,FN_CALL,CONTEXT,FN_RESULT,PARAM1,PARAM2,PARAM3)\
		 asyncall(SENDER,RECVER,FN_CALL,(asyncall_context_t)CONTEXT,FN_RESULT,(void*)PARAM1,(void*)PARAM2,(void*)PARAM3,NULL)

#define ASYNCALL4(SENDER,RECVER,FN_CALL,CONTEXT,FN_RESULT,PARAM1,PARAM2,PARAM3,PARAM4)\
		 asyncall(SENDER,RECVER,FN_CALL,(asyncall_context_t)CONTEXT,FN_RESULT,(void*)PARAM1,(void*)PARAM2,(void*)PARAM3,(void*)PARAM4,NULL)

#define ASYNRETURN(CONTEXT,RESULT) asynreturn(CONTEXT,(void*)RESULT)

#endif