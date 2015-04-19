#include <stdlib.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include "kn_exception.h"
#include "kn_except.h"
#include "log.h"

pthread_key_t g_exception_key;
#ifndef __GNUC__
pthread_once_t g_exception_key_once = PTHREAD_ONCE_INIT;
#endif

static void delete_thd_exstack(void  *arg)
{
	struct kn_exception_perthd_st *epst = (struct kn_exception_perthd_st*)arg;
	while(kn_list_size(&epst->csf_pool))
		free(kn_list_pop(&epst->csf_pool));
	free(arg);
}

int setup_sigsegv();
static void signal_segv(int signum,siginfo_t* info, void*ptr){
	kn_exception_throw(except_segv_fault,__FILE__,__FUNCTION__,__LINE__,info);
	return;
}

int setup_sigsegv(){
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	//sigaddset(&action.sa_mask,SIGINT);
	action.sa_sigaction = signal_segv;
	action.sa_flags = SA_SIGINFO;
	if(sigaction(SIGSEGV, &action, NULL) < 0) {
		perror("sigaction");
		return 0;
	}
	return 1;
}

static void signal_sigbus(int signum,siginfo_t* info, void*ptr){
	THROW(except_sigbus);
	return;
}

int setup_sigbus(){
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_sigaction = signal_sigbus;
	action.sa_flags = SA_SIGINFO;
	if(sigaction(SIGBUS, &action, NULL) < 0) {
		perror("sigaction");
		return 0;
	}
	return 1;
}

static void signal_sigfpe(int signum,siginfo_t* info, void*ptr){
	THROW(except_arith);
	return;
}

int setup_sigfpe(){
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_sigaction = signal_sigfpe;
	action.sa_flags = SA_SIGINFO;
	if(sigaction(SIGFPE, &action, NULL) < 0) {
		perror("sigaction");
		return 0;
	}
	return 1;
}

#ifndef __GNUC__
void kn_exception_once_routine(){
#else
__attribute__((constructor(103))) static void kn_exception_init(){
#endif	
	pthread_key_create(&g_exception_key,delete_thd_exstack);
	setup_sigsegv();
	setup_sigbus();
	setup_sigfpe();
}


static inline kn_callstack_frame * get_csf(kn_list *pool)
{
	int32_t i;
	kn_callstack_frame *call_frame;
	if(!kn_list_size(pool))
	{
		for(i = 0;i < 256; ++i){
			call_frame = calloc(1,sizeof(*call_frame));
			kn_list_pushfront(pool,&call_frame->node);
		}
	}
	return  (kn_callstack_frame*)kn_list_pop(pool);
}

static int addr2line(const char *addr,char *output,int size){		
	char path[256]={0};
	readlink("/proc/self/exe", path, 256);	
	char cmd[1024];
	snprintf(cmd,1024,"addr2line -fCse %s %s", path, addr);
	FILE *pipe = popen(cmd, "r");
	if(!pipe) return -1;
	int i = fread(output,1,size-1,pipe);	
	pclose(pipe);
	output[i] = '\0';
	int j = 0;
	for(; j<=i; ++j){ 
		if(output[j] == '\n') output[j] = ' ';	
	}
	return 0;
}

void kn_exception_throw(int32_t code,const char *file,const char *func,int32_t line,siginfo_t* info)
{
	void*                   bt[64];
	char**                  strings;
	size_t                  sz;
	int                     i;
	int                     sig = 0;
	kn_exception_perthd_st* epst;
	kn_callstack_frame*     call_frame;
	kn_exception_frame*     frame = kn_expstack_top();
	if(frame)
	{
		frame->exception = code;
		frame->line = line;
		frame->is_process = 0;
		if(info)frame->addr = info->si_addr;
		sz = backtrace(bt, 64);
		strings = backtrace_symbols(bt, sz);
		epst = (kn_exception_perthd_st*)pthread_getspecific(g_exception_key);
		if(code == except_segv_fault || code == except_sigbus || code == except_arith) 
			i = 3;
		else{
			i = 1;
		}
		for(; i < sz; ++i){
			if(strstr(strings[i],"exception_throw")){
				continue;
			}
			call_frame = get_csf(&epst->csf_pool);
			char *str1 = strstr(strings[i],"[");
			char *str2 = strstr(str1,"]");
			if(str1 && str2 && str2 - str1 < 128){
				int len = str2 - str1 - 1;
				char addr[128];
				strncpy(addr,str1+1,len);
				addr[len] = '\0';
				char buf[1024];
				if(0 == addr2line(addr,buf,1024)){
					snprintf(call_frame->info,1024,"%s %s\n",strings[i],buf);
				}else{
					snprintf(call_frame->info,1024,"%s\n",strings[i]);
				}				
			}else
				snprintf(call_frame->info,1024,"%s\n",strings[i]);
			kn_list_pushback(&frame->call_stack,&call_frame->node);
			if(strstr(strings[i],"main+"))
				break;
		}
		free(strings);
		if(code == except_segv_fault) sig = SIGSEGV;
		else if(code == except_sigbus) sig = SIGBUS;
		else if(code == except_arith) sig = SIGFPE;  
		siglongjmp(frame->jumpbuffer,sig);
	}
	else
	{
		sz = backtrace(bt, 64);
		strings = backtrace_symbols(bt, sz);
    		char logbuf[MAX_LOG_SIZE];
    		char *ptr = logbuf;
    		int32_t size = 0;	
    		void *addr = NULL;
    		if(info)
    			addr = info->si_addr;
    		if(code == except_segv_fault)
	    		size += snprintf(ptr,MAX_LOG_SIZE," %s [invaild access addr:%p]\n",kn_exception_description(code),addr);
    		else
	    		size += snprintf(ptr,MAX_LOG_SIZE," %s\n",kn_exception_description(code));
		ptr = logbuf + size;	    		    		
 		int f = 0;   			
		if(code == except_segv_fault || code == except_sigbus || code == except_arith) 
			i = 3;
		else{
			i = 1;
		}
		for(; i < sz; ++i){
			char *str1 = strstr(strings[i],"[");
			char *str2 = strstr(str1,"]");
			if(str1 && str2 && str2 - str1 < 128){
				int len = str2 - str1 - 1;
				char addr[128];
				strncpy(addr,str1+1,len);
				addr[len] = '\0';	
				char buf[1024];
				if(0 == addr2line(addr,buf,1024)){
	        				size += snprintf(ptr,MAX_LOG_SIZE-size,"    % 2d: %s %s\n",++f,strings[i],buf);
				}else{
	        				size += snprintf(ptr,MAX_LOG_SIZE-size,"    % 2d: %s\n",++f,strings[i]);
				}
			}else
        				size += snprintf(ptr,MAX_LOG_SIZE-size,"    % 2d: %s\n",++f,strings[i]);						
			ptr = logbuf + size;
			if(strstr(strings[i],"main+"))
				break;
		}
		SYS_LOG(LOG_ERROR,"%s",logbuf);
		free(strings);
		exit(0);
	}
}


const char* exceptions[MAX_EXCEPTION] = {
	"except_invaild_num",
	"except_alloc_failed",
	"except_list_empty",
	"except_segv_fault",
	"except_sigbus",
	"except_arith",
	"testexception3",
	NULL,
};
