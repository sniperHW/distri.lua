#include "../asyncall.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

int32_t asynreturn(asyncall_context_t context,void *result)
{
	msg_asynresult_t msg = calloc(1,sizeof(*msg));
	MSG_TYPE(msg) = MSG_ASYNRESULT;
	msg->context = context;
	if(0 != send_msg(NULL,context->caller,(msg_t)msg))
	{
		free(msg);
		return -1;
	}
	return 0;	
}

int32_t asyncall(msgdisp_t sender,msgdisp_t recver,
				 fn_asyncall fn_call,asyncall_context_t context,fn_asyncall_result fn_result,...)
{
	if(!recver || ! fn_call) return -1;
	void *param[4];
	uint8_t c = 0;
	va_list argptr;
	va_start(argptr,fn_result);
	do{
		void *tmp = va_arg(argptr,void*);
		if(tmp == NULL) break;
		if(c > 4) return -1;
		param[c++] = tmp;
	}while(1);

	msg_asyncall_t msg = calloc(1,sizeof(*msg)+sizeof(void*)*c);
	MSG_TYPE(msg) = MSG_ASYNCALL;
	msg->context = context;
	msg->fn_call = fn_call;
	if(msg->context){ 
		msg->context->fn_result = fn_result;
		msg->context->caller = sender;
	}
	if(c > 0){
		uint8_t i = 0;
		for( ;i < c; ++i)
			msg->param[i] = param[i];
	}

	if(0 != send_msg(sender,recver,(msg_t)msg))
	{
		free(msg);
		return -1;
	}
	return 0;
}