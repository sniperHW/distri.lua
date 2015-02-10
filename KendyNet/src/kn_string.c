#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "kn_string.h"
#include "kn_refobj.h"
#include "kn_common_define.h"

typedef struct string_holder
{
	refobj       ref;
	char*       str;
	uint32_t   len;
	uint32_t   cap;
}*holder_t;


static void destroy_string_holder(void *arg)
{
	holder_t h = (holder_t)arg;
	free((void*)h->str);
	free(h);
}

static inline holder_t string_holder_create(const char *str,uint32_t len)
{
	holder_t h = calloc(1,sizeof(*h));
	h->cap = size_of_pow2(len+1);
	h->str = calloc(1,h->cap);
	strncpy(h->str,str,len);
	h->str[len] = 0;
	h->len = len;
	refobj_init(&h->ref,destroy_string_holder);
	return h;
}

static inline void string_holder_append(holder_t h,const char *str,uint32_t len){
	uint32_t total_len = h->len + len +1;
	if(h->cap < total_len){
		h->cap = size_of_pow2(total_len);
		char *tmp = realloc(h->str,h->cap);
		if(!tmp){
			tmp = calloc(1,h->cap);
			strcpy(tmp,h->str);
			free(h->str);
		}
		h->str = tmp;
	}
	strcat(h->str,str);
	h->len = total_len - 1;
}

static inline void string_holder_replace(holder_t h,const char *str,uint32_t n){
	uint32_t total_len = n +1;
	if(h->cap < total_len){
		h->cap = size_of_pow2(total_len);
		char *tmp = realloc(h->str,h->cap);
		if(!tmp){
			tmp = calloc(1,h->cap);
			free(h->str);
		}
		h->str = tmp;				
	}
	strncpy(h->str,str,n);
	h->str[n] = 0;
	h->len = n;	
}

static inline void string_holder_release(holder_t h)
{
	if(h) refobj_dec(&h->ref);
}

static inline void string_holder_acquire(holder_t h)
{
    if(h) refobj_inc(&h->ref);
}


#define MIN_STRING_LEN 64

struct kn_string
{
	holder_t holder;
	uint32_t len;
	char str[MIN_STRING_LEN];
};


kn_string_t kn_new_string(const char *str)
{
	if(!str) return NULL;
	kn_string_t _str = calloc(1,sizeof(*_str));
	uint32_t len = strlen(str);
	if(len + 1 <= MIN_STRING_LEN){
		strcpy(_str->str,str);
		_str->len = len;
	}
	else
		_str->holder = string_holder_create(str,len);
	return _str;
}

void kn_release_string(kn_string_t s)
{
	if(s->holder) string_holder_release(s->holder);
	free(s);
}

const char *kn_to_cstr(kn_string_t s){
	if(s->holder) return s->holder->str;
	return s->str;
}

int32_t  kn_string_len(kn_string_t s)
{
	if(s->holder) return s->holder->len;
	return s->len;
}

void kn_string_replace(kn_string_t to,const char *from,uint32_t n)
{
	uint32_t _n = strlen(from);
	if(n > _n) n = _n;
	if(to->holder){
		string_holder_replace(to->holder,from,n);
	}else if(n + 1 <= MIN_STRING_LEN){
		strncpy(to->str,from,n);
		to->str[n] = 0;
		to->len = n;
	}else{
		to->holder = string_holder_create(from,n);
	}
}

void kn_string_append(kn_string_t s,const char *str)
{
	if(!s || !str) return;
	uint32_t len = strlen(str);
	uint32_t total_len = kn_string_len(s) + len + 1;
	if(s->holder)
		string_holder_append(s->holder,str,len);	
	else if(total_len <= MIN_STRING_LEN){
		strcat(s->str,str);
		s->len = total_len - 1;
	}
	else{
		s->holder = string_holder_create(kn_to_cstr(s),s->len);
		string_holder_append(s->holder,str,len);
	}
}
