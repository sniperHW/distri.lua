#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "hash_map.h"

enum
{
	ITEM_EMPTY = 0,
	ITEM_DELETE,
	ITEM_USED,
};



#define GET_KEY(HASH_MAP,ITEM) ((void*)(ITEM)->key_and_val)

#define GET_VAL(HASH_MAP,ITEM) ((void*)&(ITEM)->key_and_val[(HASH_MAP)->key_size])

#define GET_ITEM(ITEMS,ITEM_SIZE,SLOT) ((struct hash_item*)&((int8_t*)(ITEMS))[ITEM_SIZE*SLOT])

#define HASH_MAP_INDEX(HASH_CODE,SLOT_SIZE) (HASH_CODE % SLOT_SIZE)



hash_map_t hash_map_create(uint32_t slot_size,uint32_t key_size,
	uint32_t val_size,hash_func hash_function,hash_key_eq key_cmp_function)
{
	hash_map_t h = (hash_map_t)malloc(sizeof(*h));
	if(!h)
		return 0;
	h->slot_size = slot_size;
	h->size = 0;
	h->_hash_function = hash_function;
	h->_key_cmp_function = key_cmp_function;
	h->key_size = key_size;
	h->val_size = val_size;
	h->item_size = sizeof(struct hash_item) + key_size + val_size;
	h->_items = calloc(slot_size,h->item_size);
	h->expand_size = h->slot_size - h->slot_size/4;
	dlist_init(&h->dlink);
	if(!h->_items)
	{
		free(h);
		return 0;
	}
	return h;
}

void hash_map_destroy(hash_map_t *h)
{
	free((*h)->_items);
	free(*h);
	*h = 0;
}

void hash_map_iter_get_val(struct base_iterator *_iter, void *val)
{
	hash_map_iter *iter = (hash_map_iter*)_iter;
	hash_map_t h = (hash_map_t)iter->data1;
	struct hash_item *item = (struct hash_item *)iter->data2;
	memcpy(val,GET_VAL(h,item),h->val_size);
}

void hash_map_iter_set_val(struct base_iterator *_iter, void *val)
{
	hash_map_iter *iter = (hash_map_iter*)_iter;
	hash_map_t h = (hash_map_t)iter->data1;
	struct hash_item *item = (struct hash_item *)iter->data2;
	void *ptr = (void*)&((item)->key_and_val[(h)->key_size]);
	memcpy(ptr,val,h->val_size);
}

void hash_map_iter_next(struct base_iterator *_iter)
{
	hash_map_iter *iter = (hash_map_iter*)_iter;
	
	struct hash_item *item = iter->data2;
	hash_map_t h = iter->data1;
	if(!item || !h)
		return;
	if(item == (struct hash_item*)dlist_last(&h->dlink))
	{
		iter->data1 = iter->data2 = NULL;
		return;
	}
	iter->data1 = h;
	iter->data2 = item->dnode.next;
}

int8_t hash_map_iter_equal(struct base_iterator *_a,struct base_iterator *_b)
{
	hash_map_iter *a = (hash_map_iter*)_a;
	hash_map_iter *b = (hash_map_iter*)_b;
	return a->data1 == b->data1 && a->data2 == b->data2;
}

void hash_map_iter_init(hash_map_iter *iter,void *data1,void *data2)
{
	iter->base.next = hash_map_iter_next;
	iter->base.get_key = NULL;
	iter->base.get_val = hash_map_iter_get_val;
	iter->base.set_val = hash_map_iter_set_val;
	iter->base.is_equal = hash_map_iter_equal;
	iter->data1 = data1;
	iter->data2 = data2;
}

static inline struct hash_item *_hash_map_insert(hash_map_t h,void* key,void* val,uint64_t hash_code)
{
	uint32_t slot = HASH_MAP_INDEX(hash_code,h->slot_size);
	uint32_t check_count = 0;
	struct hash_item *item = 0;
	while(check_count < h->slot_size)
	{
		item = GET_ITEM(h->_items,h->item_size,slot);
		if(item->flag != ITEM_USED)
		{
			//找到空位置了
			item->flag = ITEM_USED;
			memcpy(item->key_and_val,key,h->key_size);
			memcpy(&(item->key_and_val[h->key_size]),val,h->val_size);
			item->hash_code = hash_code;
			//printf("check_count:%d\n",check_count);
			++h->size;
			dlist_push(&h->dlink,(struct dnode*)item);
			return item;
		}
		else
			if(hash_code == item->hash_code && h->_key_cmp_function(key,GET_KEY(h,item)) == 0)
				break;//已经在表中存在
		slot = (slot + 1)%h->slot_size;
		check_count++;
	}
	//插入失败
	return NULL;
}

static inline int32_t _hash_map_expand(hash_map_t h)
{
	uint32_t old_slot_size = h->slot_size;
	struct hash_item *old_items = h->_items;
	uint32_t i = 0;
	h->slot_size <<= 1;
	h->_items = calloc(h->slot_size,h->item_size);
	if(!h->_items)
	{
		h->_items = old_items;
		h->slot_size >>= 1;
		return -1;
	}
	h->size = 0;
	dlist_init(&h->dlink);
	for(; i < old_slot_size; ++i)
	{
		struct hash_item *_item = GET_ITEM(old_items,h->item_size,i);
		if(_item->flag == ITEM_USED)
			_hash_map_insert(h,GET_KEY(h,_item),GET_VAL(h,_item),_item->hash_code);
	}
	h->expand_size = h->slot_size - h->slot_size/4;
	free(old_items);
	return 0;
}

hash_map_iter hash_map_insert(hash_map_t h,void *key,void *val)
{
	uint64_t hash_code = h->_hash_function(key);
	if(h->slot_size < 0x80000000 && h->size >= h->expand_size)
		//空间使用超过3/4扩展
		_hash_map_expand(h);
	CREATE_HASH_IT(iter,NULL,NULL);		
	if(h->size >= h->slot_size)
		return iter;
	struct hash_item *item = _hash_map_insert(h,key,val,hash_code);	
	if( item != NULL)
	{
		iter.data1 = h;
		iter.data2 = item;
	}
	return iter;
	
}
static inline struct hash_item *_hash_map_find(hash_map_t h,void *key)
{
	uint64_t hash_code = h->_hash_function(key);
	uint32_t slot = HASH_MAP_INDEX(hash_code,h->slot_size);
	uint32_t check_count = 0;
	struct hash_item *item = 0;
	while(check_count < h->slot_size)
	{
		item = GET_ITEM(h->_items,h->item_size,slot);
		if(item->flag == ITEM_EMPTY)
			return NULL;
		if(item->hash_code == hash_code && h->_key_cmp_function(key,GET_KEY(h,item)) == 0)
		{
			if(item->flag == ITEM_DELETE)
				return NULL;
			else
				return item;
		}
		slot = (slot + 1)%h->slot_size;
		check_count++;
	}
	return NULL;
}

hash_map_iter hash_map_find(hash_map_t h,void* key)
{
	struct hash_item *item = _hash_map_find(h,key);
	CREATE_HASH_IT(iter,NULL,NULL);	
	if(item)
	{
		iter.data1 = h;
		iter.data2 = item;
	}
	return iter;
}

void* hash_map_remove(hash_map_t h,void* key)
{
	struct hash_item *item = _hash_map_find(h,key);
	if(item)
	{
		item->flag = ITEM_DELETE;
		--h->size;
		dlist_remove((struct dnode*)item);
		return GET_VAL(h,item);
	}
	return NULL;
}

void* hash_map_erase(hash_map_t h,hash_map_iter iter)
{
	if(iter.data1 && iter.data2)
	{
		hash_map_t h = (hash_map_t)iter.data1;
		struct hash_item *item = (struct hash_item *)iter.data2;
		item->flag = ITEM_DELETE;
		--h->size;
		return GET_VAL(h,item);	
	}	
	return NULL;
}
