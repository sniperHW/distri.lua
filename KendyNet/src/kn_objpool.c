/*
* 非线程安全的对象分池
*/

#include "kn_objpool.h"
#include "kn_list.h"
#include "kn_common_define.h"
#include <string.h>
#include <assert.h>

#pragma pack(1)
typedef struct{
	uint32_t next:10;
	uint32_t chunkidx:22;
	uint16_t idx;
#ifdef _DEBUG
	allocator_t    _allocator;
#endif
	char data[0];
}obj;
#pragma pack()

typedef struct{
	kn_list_node _lnode;
	uint16_t head;
	char data[0];
}chunk;

typedef struct {
	allocator base;
	size_t         objsize;
	uint32_t     initnum;
	chunk**    chunks;
	uint32_t     chunkcount;
	int8_t          usesystem;
	kn_list        freechunk;
}*objpool_t;

#define CHUNK_SIZE 1024
#define CHUNK_OBJSIZE (CHUNK_SIZE-1)
#define MAX_CHUNK 0x3FFFFF

static chunk *new_chunk(uint32_t idx,size_t objsize){
	chunk* newchunk = calloc(1,sizeof(*newchunk) + CHUNK_SIZE*objsize);
	uint32_t i = 1;
	char *ptr = newchunk->data + objsize;
	for(; i < CHUNK_SIZE;++i){
		obj *_obj = (obj*)ptr;
		_obj->chunkidx = idx;
		if(i == CHUNK_OBJSIZE)
			_obj->next = 0;
		else
			_obj->next = i + 1;
		_obj->idx = i;
		ptr += objsize;		
	}
	newchunk->head = 1;
	return newchunk;
}

static void* _alloc(allocator_t _,size_t size){
	objpool_t a = (objpool_t)_;	
	if(unlikely(a->usesystem)) return malloc(size);
	return ((allocator_t)a)->calloc(_,1,1);
}

static void* _calloc(allocator_t _,size_t num,size_t size){
	objpool_t a = (objpool_t)_;
	if(unlikely(a->usesystem)) return calloc(num,size);
	chunk *freechunk = (chunk*)kn_list_head(&a->freechunk);
	uint32_t objsize = a->objsize;
	if(unlikely(!freechunk)){
		uint32_t chunkcount = a->chunkcount*2;
		if(chunkcount > MAX_CHUNK) chunkcount = MAX_CHUNK;
		if(chunkcount == a->chunkcount) return NULL;
		chunk **tmp = calloc(chunkcount,sizeof(*tmp));
		uint32_t i = 0;
		for(; i < a->chunkcount;++i){
			tmp[i] = a->chunks[i];
		}
		i = a->chunkcount;
		for(; i < chunkcount;++i)	{
			tmp[i] = new_chunk(i,objsize);
			kn_list_pushback(&a->freechunk,(kn_list_node*)tmp[i]);			
		}
		free(a->chunks);
		a->chunks = tmp;
		a->chunkcount = chunkcount;
		freechunk = (chunk*)kn_list_head(&a->freechunk);
	}
	obj *_obj = (obj*)(((char*)freechunk->data) + freechunk->head * objsize);
	if(unlikely(!(freechunk->head = _obj->next)))
		kn_list_pop(&a->freechunk);	
	memset(_obj->data,0,objsize-sizeof(*_obj));
#ifdef _DEBUG
	_obj->_allocator = (allocator_t)a;
#endif	
	return (void*)_obj->data;
}


static void   _free(allocator_t _,void *ptr){
	objpool_t a = (objpool_t)_;	
	if(unlikely(a->usesystem)) return free(ptr);
	obj *_obj = (obj*)((char*)ptr - sizeof(obj));
#ifdef _DEBUG
	assert(_obj->_allocator == (allocator_t)a);
#endif
	chunk *_chunk = a->chunks[_obj->chunkidx];
	uint32_t index = _obj->idx;
	assert(index >=1 && index <= CHUNK_OBJSIZE);
	_obj->next = _chunk->head;
	if(unlikely(!_chunk->head))
		kn_list_pushback(&a->freechunk,(kn_list_node*)_chunk);
	_chunk->head = index;	
}

allocator_t objpool_new(size_t objsize,uint32_t initnum){
	uint32_t chunkcount;
	if(initnum%CHUNK_OBJSIZE == 0)
		chunkcount = initnum/CHUNK_OBJSIZE;
	else
		chunkcount = initnum/CHUNK_OBJSIZE + 1;
	if(chunkcount > MAX_CHUNK) return NULL;		
	objsize = align_size(sizeof(obj) + objsize,8);
	objpool_t pool = calloc(1,sizeof(*pool));
	do{
		((allocator_t)pool)->alloc = _alloc;
		((allocator_t)pool)->calloc = _calloc;
		((allocator_t)pool)->free = _free;
		if(objsize > 1024*1024){
			pool->usesystem = 1;
			break;
		}
		pool->objsize = objsize;
		pool->chunkcount = chunkcount;
		pool->chunks = calloc(chunkcount,sizeof(*pool->chunks));
		uint32_t i = 0;
		for(; i < chunkcount; ++i){
			pool->chunks[i] = new_chunk(i,objsize);
			kn_list_pushback(&pool->freechunk,(kn_list_node*)pool->chunks[i]);
		}
	}while(0);
	return (allocator_t)pool;
}

void objpool_destroy(allocator_t _){
	objpool_t a = (objpool_t)_;		
	uint32_t i = 0;
	for(; i < a->chunkcount; ++i){
		free(a->chunks[i]);
	}
	free(a->chunks);
	free(a);
}