#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "common_hash_function.h"
#include "hash_map.h"

struct st{
	hash_node base;
	char key[32];
};

static uint64_t hash(void *key)
{
	return burtle_hash((uint8_t*)key,strlen((char*)key),0);
}

static uint64_t snd_hash(void *key)
{
	return burtle_hash((uint8_t*)key,strlen((char*)key),1);
}

static int key_cmp_function(void *key1,void *key2){
	return strcmp((char*)key1,(char*)key2);
}


/*static  void    ondestroy(hash_node *n)
{
	free(n);
}*/



int main(){
	
	hash_map_t h = hash_map_create(512,hash,key_cmp_function,snd_hash);
	int i = 0;
	for(; i < 10000; ++i){
		struct st *tmp = calloc(1,sizeof(*tmp));
		tmp->base.key = tmp->key;
		snprintf(tmp->key,32,"key%d",i);
		hash_map_insert(h,(hash_node*)tmp);
	}
	
	
	free(hash_map_remove(h,"key5"));
	free(hash_map_remove(h,"key15"));
	free(hash_map_remove(h,"key22"));
	free(hash_map_remove(h,"key35"));
	free(hash_map_remove(h,"key525"));
	free(hash_map_remove(h,"key528"));
	free(hash_map_remove(h,"key1788"));
	free(hash_map_remove(h,"key1709"));
	free(hash_map_remove(h,"key1005"));
	free(hash_map_remove(h,"key2013"));

	int c = 0;
	for(i = 0; i < 10000; ++i){
		char key[32];
		snprintf(key,32,"key%d",i);
		struct st *_1 = (struct st*)hash_map_find(h,key);
		if(!_1){
			printf("%s miss\n",key);
		}else{
			++c;
		}
	}
	
	
	if(c == 10000-10)
		printf("pass\n");
	
	/*free(hash_map_remove(h,"key5"));
		
	struct st *cur;
	for_each(h,struct st*,cur){
		printf("%s\n",cur->key);
	}*/
		
	return 0;
}	 
