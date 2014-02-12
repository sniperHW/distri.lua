#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "kn_string.h"
#include "refbase.h"
#include "buffer.h"

typedef struct string_holder
{
	struct refbase ref;
	char       *str;
	int32_t     len;
}*holder_t;


void destroy_string_holder(void *arg)
{
	printf("destroy_string_holder\n");
	holder_t h = (holder_t)arg;
	free((void*)h->str);
	free(h);
}

static inline holder_t string_holder_create(const char *str,uint32_t len)
{
	holder_t h = calloc(1,sizeof(*h));
	if(strlen(str)+1 == len){	
		h->str = calloc(1,size_of_pow2(len));
		strncpy(h->str,str,len);
	}else{
		h->str = calloc(1,size_of_pow2(len+1));
		strncpy(h->str,str,len);
		h->str[len] = 0;
	}
	ref_init(&h->ref,0,destroy_string_holder,1);
	return h;
}

static inline void string_holder_release(holder_t h)
{
	if(h) ref_decrease(&h->ref);
}

static inline void string_holder_acquire(holder_t h)
{
    if(h) ref_increase(&h->ref);
}


#define MIN_STRING_LEN 64

struct string
{
	holder_t holder;
	char str[MIN_STRING_LEN];
};


string_t new_string(const char *str)
{
	if(!str) return NULL;

	string_t _str = calloc(1,sizeof(*_str));
	int32_t len = strlen(str)+1;
	if(len <= MIN_STRING_LEN)
		strncpy(_str->str,str,MIN_STRING_LEN);
	else
		_str->holder = string_holder_create(str,len);
	return _str;
}

void release_string(string_t s)
{
	if(s->holder) string_holder_release(s->holder);
	free(s);
}

const char *to_cstr(string_t s){
	if(s->holder) return s->holder->str;
	return s->str;
}

int32_t  string_len(string_t s)
{
	if(s->holder) return strlen(s->holder->str)+1;
	return strlen(s->str)+1;
}

void string_copy(string_t to,string_t from,uint32_t n)
{
	if(n > string_len(from)) return;
	if(n <= MIN_STRING_LEN)
	{
		if(to->holder){
			string_holder_release(to->holder);
			to->holder = NULL;
		}
		char *str = from->str;
		if(from->holder) str = from->holder->str;
		strncpy(to->str,str,n);
	}else
	{
		if(n == string_len(from)){
			if(to->holder) string_holder_release(to->holder);
			string_holder_acquire(to->holder);
		}else if(n > string_len(to)){
			if(to->holder) string_holder_release(to->holder);
			to->holder = string_holder_create(to_cstr(from),n);	
		}else{
			strncpy(to->holder->str,to_cstr(from),n);	
		}
	}
}

void string_append(string_t s,const char *cstr)
{
	if(!s || !cstr) return;
	int32_t len = string_len(s) + strlen(cstr);
	if(len <= MIN_STRING_LEN)
		strcat(s->str,cstr);
	else
	{
		if(s->holder && len <= s->holder->len)
			strcat(s->holder->str,cstr);
		else{
			
			holder_t h = string_holder_create(to_cstr(s),len*2);
			strcat(h->str,cstr);
			string_holder_release(s->holder);
			s->holder = h;
		}
	}
}