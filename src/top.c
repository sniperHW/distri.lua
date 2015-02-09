#include "top.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

struct top{
	char **filter;
	int     filtercap;
};

struct top* new_top(){
	return calloc(1,sizeof(struct top));
}

void destroy_top(struct top *t){
	if(t->filter){
		int i = 0;
		for(; i < t->filtercap; ++i){
			if(t->filter) free(t->filter[i]);
		}
		free(t->filter);
	}
	free(t);
}

void    add_filter(struct top *t,const char *filter){
	if(!t->filter){
		t->filtercap = 4;
		t->filter = calloc(t->filtercap,sizeof(*t->filter));
	}
	int i = 0;
	for(; i < t->filtercap; ++i){
		if(!t->filter[i]) break;
	}
	if(i == t->filtercap){
		//expand
		int newcap = t->filtercap << 2;
		char **tmp = calloc(newcap,sizeof(*tmp));
		int j = 0;
		for( ; j < i; ++j){
			tmp[j] = t->filter[j];
		}
		free(t->filter);
		t->filter = tmp;
	}
	char *str = calloc(strlen(filter) + 1,sizeof(*str));
	strcpy(str,filter);
	t->filter[i] = str;
}

int top_match(struct top *t,const char *line){
	if(!t->filter) return 0;
	int i = 0;
	for(; i < t->filtercap; ++i){
		if(t->filter[i] && strstr(line,t->filter[i]))
			return 1;
	}	
	return 0;
}

//PID USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND
//need field 1,2,9,10,12

void top_format(char *line,int len){
	//format to below
	//pid:12608,usr:sniper,cpu:0.98,mem:1.27MB,cmd:./testtop
#ifdef _LINUX
	//in linux fetch 1,2,9,10,12	
	static const char *field_name[] = {
		NULL,
		"pid:",
		"usr:",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		"cpu:",
		"mem:",
		NULL,
		"cmd:"
	};
#elif _BSD
	static const char *field_name[] = {
		NULL,
		"pid:",
		"usr:",
		NULL,
		NULL,
		NULL,
		NULL,
		"mem:",
		NULL,
		NULL,
		NULL,
		"cpu:",		
		"cmd:"
	};	
#else
	#error "un support platform!"
#endif	

	char *tmp = calloc(len,sizeof(*tmp));
	memcpy(tmp,line,len);
	int idx = 0;
	int field_begin = 0;
	int i = 0;
	int match = 0;
	char *ptr = line;
	for(; i < len; ++i){
		if(tmp[i] == ' ' && idx != 12){
			if(!field_begin) 
				continue;
			else{
				field_begin = 0;
				if(match){
#ifdef _LINUX					
					if(0 == strcmp(field_name[idx],"cpu:") ||
					    0 == strcmp(field_name[idx],"mem:")){
						*ptr++ = '%';
					 }
#endif					 					
					*ptr++ = ',';
					match = 0;
				}
			}
		}else if(tmp[i] == '\n'){
			break;
		}
		else{
			if(!field_begin){ 
				field_begin = 1;
				++idx;
				if(field_name[idx]){
					match = 1;
					int _len = strlen(field_name[idx]);
					memcpy(ptr,field_name[idx],_len);
					ptr += _len;	
				}
			}
			if(match) *ptr++ = tmp[i];
		}
	}
	*ptr++ = '\n';
	*ptr = 0;
	free(tmp);
}

kn_string_t get_top(struct top *t){
#ifdef _LINUX	
	FILE *pipe = popen("top -b -n 1 -c -w 512", "r");
#elif _BSD	
	FILE *pipe = popen("top -b -n 1000 -C -w 512", "r");
#endif	
	if(!pipe) return NULL;
	char ch = fgetc(pipe);
	int i=0;
	kn_string_t ret = kn_new_string("");
	char line[65536];
	int flag = 0;
#ifdef _BSD
	int c = 0;
#endif
	while(ch != EOF){
		line[i++] = ch;
		if(ch == '\n'){
			line[i] = 0;
			if(flag){
				if(top_match(t,line)){
					top_format(line,65536);
					kn_string_append(ret,line);
				}
			}else if(i == 1){
#ifdef _BSD
				if(++c == 3){
					flag = 1;
					kn_string_append(ret,"process_info\n");
				}
#else
				flag = 1;
				kn_string_append(ret,"process_info\n");
#endif
			}else{
				kn_string_append(ret,line);
			}
			i = 0;
		}
		ch = fgetc(pipe);
	}
	pclose(pipe);
	return ret;
}
