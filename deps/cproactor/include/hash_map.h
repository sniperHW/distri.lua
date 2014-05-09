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
#ifndef _HASH_MAP_H
#define _HASH_MAP_H
#include <stdint.h>
#include "common_hash_function.h"
#include "kn_dlist.h"
#include "iterator.h"
//typedef struct hash_map* hash_map_t;

typedef uint64_t (*hash_func)(void*);
typedef int32_t (*hash_key_eq)(void*,void*);

typedef struct 
{
	struct  base_iterator base;
	void    *data1;
	void    *data2;
}hash_map_iter;

struct hash_item
{
	kn_dlist_node dnode;
	uint64_t hash_code;
	int8_t flag;
	int8_t key_and_val[0];
};

typedef struct hash_map
{
	hash_func   _hash_function;
	hash_key_eq _key_cmp_function;
	uint32_t slot_size;
	uint32_t size;
	uint32_t key_size;
	uint32_t val_size;
	uint32_t item_size;
	uint32_t expand_size;
	struct hash_item *_items;
	kn_dlist dlink;
}*hash_map_t;


hash_map_t     hash_map_create(uint32_t slot_size,uint32_t key_size,uint32_t val_size,hash_func,hash_key_eq);
void           hash_map_destroy(hash_map_t*);
hash_map_iter  hash_map_insert(hash_map_t,void *key,void *val);
void*          hash_map_remove(hash_map_t,void* key);

hash_map_iter  hash_map_find(hash_map_t,void* key); 
void*          hash_map_erase(hash_map_t,hash_map_iter);


void hash_map_iter_init(hash_map_iter *,void *,void *);
#define CREATE_HASH_IT(IT,ARG1,ARG2)\
	hash_map_iter IT;\
	hash_map_iter_init(&IT,ARG1,ARG2)

static inline hash_map_iter  hash_map_begin(hash_map_t h)
{
	CREATE_HASH_IT(iter,NULL,NULL);	
	struct hash_item *item = (struct hash_item*)kn_dlist_first(&h->dlink);
	if( item != NULL)
	{
		iter.data1 = h;
		iter.data2 = item;
	}
	return iter;
}

static inline hash_map_iter  hash_map_end(hash_map_t h)
{
	CREATE_HASH_IT(iter,NULL,NULL);	
	return iter;
}

static inline int32_t hash_map_size(hash_map_t h)
{
	return h->size;
}

#ifndef HASH_MAP_INSERT
#define HASH_MAP_INSERT(KEY_TYPE,VAL_TYPE,HASH_MAP,KEY,VAL)\
	({ hash_map_iter ret; KEY_TYPE __key = KEY;VAL_TYPE __val = VAL;\
       do ret = hash_map_insert(HASH_MAP,&__key,&__val);\
       while(0);\
       ret;})		
#endif

#ifndef HASH_MAP_REMOVE
#define HASH_MAP_REMOVE(KEY_TYPE,HASH_MAP,KEY)\
	({ void* ret; KEY_TYPE __key = KEY;\
       do ret = hash_map_remove(HASH_MAP,&__key);\
       while(0);\
       ret;})		
#endif

#ifndef HASH_MAP_FIND
#define HASH_MAP_FIND(KEY_TYPE,HASH_MAP,KEY)\
	({ hash_map_iter ret; KEY_TYPE __key = KEY;\
       do ret = hash_map_find(HASH_MAP,&__key);\
       while(0);\
       ret;})		
#endif
#endif
