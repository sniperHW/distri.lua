#include "top.h"
#include <stdio.h>

struct top{
	char **filter;
	int     filtercap;
};

struct top* new_top(){
	return calloc(struct top);
}

void destroy_top(struct top *t){
	if(filter){
		int i = 0;
		for(; i < t->filtercap; ++i){
			if(t->filter) free(t->filter[i]);
		}
		free(filter);
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
}

kn_string_t get_top(struct top *t){
	FILE *pipe = popen("top -b -n 1 -c", "r");
	if(!pipe) return NULL;
	char ch = fgetc(pipe);
	int i;
	kn_string_t ret = kn_new_string("");
	char line[65536];
	int flag = 0;
	while(ch != EOF){
		line[i++] = ch;
		if(ch == '\n'){
			if(flag){
				line[i] = 0;
				if(top_match(line)){
					top_format(line,65536);
					kn_string_append(ret,line);
				}
			}else if(i == 1){
				flag = 1;
				kn_string_append(ret,"process_info\n");
			}
			i = 0;
		}
		ch = fgetc(pipe);
		putchar(ch);
	}
	pclose(pipe);
	return ret;
}