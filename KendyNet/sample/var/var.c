#include <stdio.h>
#include <stdarg.h>
#include "var.h"
#include "common_hash_function.h"

struct var{
	hash_node hash;
	union{
		double          d;
		uint64_t        n;
		kn_string_t    s;
		hash_map_t  t;
	};
	uint8_t  tt;
};

static uint64_t var_hash_func(void *_){
	
	//return burtle_hash((uint8_t*)key,32,1);
}

static uint64_t var_snd_hash_func(void *key){
	
	//return burtle_hash((uint8_t*)key,32,2);
}

static int var_key_cmp(void *_1,void *_2){
	

	//return strncmp((char*)_1,(char*)_2,32);
} 

var_t new_var(uint8_t tt,...){
	va_list vl;
	va_start(vl,tt);	
	var_t v = calloc(1,sizeof(*v));
	switch(tt){
		case VAR_8:
		case VAR_16:
		case VAR_32:
		case VAR_64:
		{
			v->n = va_arg(vl,uint64_t);
			break;	
		}
		case VAR_DOUBLE:
		{
			v->d = va_arg(vl,double);
			break;
		}
		case VAR_STR:
		{
			const char *str = va_arg(vl,const char *);
			v->s = kn_new_string(str);
			break;
		}
		case VAR_TABLE:
		{
			v->t = hash_map_create(16,var_hash_func,var_key_cmp,var_snd_hash_func);
			break;
		} 
		default:{
			free(v);
			v = NULL;
			break;
		}
	}
	return v;
}

void release_var(var_t v){
	switch(v->tt){
		case VAR_8:
		case VAR_16:
		case VAR_32:
		case VAR_64:
		case VAR_DOUBLE:
		{	
			break;
		}
		case VAR_STR:{
			kn_release_string(v->s);
			v->s = NULL;
			break;
		}
		case VAR_TABLE:{
			hash_map_destroy(v->t,(hash_destroy)release_var);
			v->t = NULL;
			break;
		}
		default:{
			return;
		}
	}
	free(v);	
}