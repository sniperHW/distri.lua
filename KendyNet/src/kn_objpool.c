#include "kn_objpool.h"
#include "kn_list.h"
#include "kn_common_define.h"
#include <string.h>
#include <assert.h>

typedef struct{
	kn_list_node _lnode;
	uint32_t chunkidx;
#ifdef _DEBUG
	allocator_t    _allocator;
#endif
	char data[0];
}obj;


typedef struct{
	kn_list_node _lnode;
	kn_list freelist;
	uint32_t        cap;
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

#define CHUNK_SIZE 4096*1024

static chunk *new_chunk(uint32_t idx,size_t objsize,uint32_t initnum){
	uint32_t totalsize = objsize * initnum;
	if(totalsize < CHUNK_SIZE)
		totalsize = CHUNK_SIZE;
	chunk* newchunk = calloc(1,sizeof(*newchunk) + totalsize);
	newchunk->cap = totalsize/objsize;
	uint32_t i = 0;
	char *ptr = newchunk->data;
	for(; i < newchunk->cap; ++i){
		obj *_obj = (obj*)ptr;
		_obj->chunkidx = idx;
		kn_list_pushback(&newchunk->freelist,(kn_list_node*)_obj);
		ptr += objsize;
	}
	return newchunk;
}

static void* _alloc(allocator_t _,size_t size){
	objpool_t a = (objpool_t)_;	
	if(a->usesystem) return malloc(size);
	return ((allocator_t)a)->calloc(_,1,1);
}

static void* _calloc(allocator_t _,size_t num,size_t size){
	objpool_t a = (objpool_t)_;
	if(a->usesystem) return calloc(num,size);
	chunk *freechunk = (chunk*)kn_list_pop(&a->freechunk);
	if(!freechunk){
		uint32_t i = 0;
		for(; i < a->chunkcount; ++i){
			if(!a->chunks[i]) break;
		}
		if(i >= a->chunkcount){
			chunk **tmp = calloc(a->chunkcount*2,sizeof(*tmp));
			uint32_t j = 0;
			for(; j < a->chunkcount;++j){
				tmp[j] = a->chunks[j];
			}
			free(a->chunks);
			a->chunks = tmp;
			a->chunkcount *= 2;
		}
		a->chunks[i] = new_chunk(i,a->objsize,a->initnum);
		freechunk = a->chunks[i];
	}
	obj *_obj = (obj*)kn_list_pop(&freechunk->freelist);
	if(kn_list_size(&freechunk->freelist) != 0){
		kn_list_pushback(&a->freechunk,(kn_list_node*)freechunk);
	}
	memset(_obj->data,0,a->objsize-sizeof(*_obj));
#ifdef _DEBUG
	_obj->_allocator = (allocator_t)a;
#endif	
	return (void*)_obj->data;
}


static void   _free(allocator_t _,void *ptr){
	objpool_t a = (objpool_t)_;	
	if(a->usesystem) return free(ptr);
	obj *_obj = (obj*)((char*)ptr - sizeof(obj));
#ifdef _DEBUG
	assert(_obj->_allocator == (allocator_t)a);
#endif
	chunk *_chunk = a->chunks[_obj->chunkidx];
	kn_list_pushback(&_chunk->freelist,(kn_list_node*)_obj);

	uint32_t freesize = kn_list_size(&_chunk->freelist);
	//if(freesize == _chunk->cap){

	/*}else*/ 
	if(freesize == 1){
		kn_list_pushback(&a->freechunk,(kn_list_node*)_chunk);
	}
}

allocator_t objpool_new(size_t objsize,uint32_t initnum){
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
		pool->chunkcount = 1;
		pool->chunks = calloc(pool->chunkcount,sizeof(*pool->chunks));
		pool->chunks[0] = new_chunk(0,objsize,initnum);
		kn_list_pushback(&pool->freechunk,(kn_list_node*)pool->chunks[0]);
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

/*
#include "kn_objpool.h"
#include "kn_list.h"
#include "kn_common_define.h"
#include <string.h>
#include <assert.h>

#pragma pack(4)
typedef struct{
	uint32_t next:8;
	uint32_t chunkidx:24;
#ifdef _DEBUG
	allocator_t    _allocator;
#endif
	char data[0];
}obj;
#pragma pack()

typedef struct{
	kn_list_node _lnode;
	uint32_t objsize;
	uint8_t head;
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

#define CHUNK_SIZE 4096*1024

obj *chunk_alloc(chunk *_chunk){
	if(!_chunk->head) return NULL;
	obj *_obj = (obj*)(((char*)_chunk->data) + _chunk->head * _chunk->objsize);
	_chunk->head = _obj->next;
	return _obj;
}

void chunk_dealloc(chunk *_chunk,obj *_obj){
	uint32_t index = ((char*)_obj - _chunk->data)/_chunk->objsize;
	assert(index >=1 && index <= 255);
	_obj->next = _chunk->head;
	_chunk->head = index;
}

static chunk *new_chunk(uint32_t idx,size_t objsize){
	chunk* newchunk = calloc(1,sizeof(*newchunk) + 256*objsize);
	int i = 1;
	char *ptr = newchunk->data + objsize;
	for(; i < 256;++i){
		obj *_obj = (obj*)ptr;
		_obj->chunkidx = idx;
		if(i == 255)
			_obj->next = 0;
		else
			_obj->next = i + 1;
		ptr += objsize;		
	}
	newchunk->head = 1;
	return newchunk;
}

static void* _alloc(allocator_t _,size_t size){
	objpool_t a = (objpool_t)_;	
	if(a->usesystem) return malloc(size);
	return ((allocator_t)a)->calloc(_,1,1);
}

static void* _calloc(allocator_t _,size_t num,size_t size){
	objpool_t a = (objpool_t)_;
	if(a->usesystem) return calloc(num,size);
	chunk *freechunk = (chunk*)kn_list_pop(&a->freechunk);
	if(!freechunk){
		uint32_t chunkcount = a->chunkcount*2;
		if(chunkcount > 0xFFFFFF) chunkcount = 0xFFFFFF;
		if(chunkcount == a->chunkcount) return NULL;
		chunk **tmp = calloc(chunkcount,sizeof(*tmp));
		uint32_t i = 0;
		for(; i < a->chunkcount;++i){
			tmp[i] = a->chunks[i];
		}
		i = a->chunkcount;
		for(; i < chunkcount;++i)	{
			a->chunks[i] = new_chunk(i,a->objsize);
			kn_list_pushback(&a->freechunk,(kn_list_node*)pool->chunks[i]);			
		}
		freechunk = (chunk*)kn_list_pop(&a->freechunk);
	}
	obj *_obj = chunk_alloc(freechunk);
	if(!freechunk->head)
		kn_list_pushback(&a->freechunk,(kn_list_node*)freechunk);
	}
	memset(_obj->data,0,a->objsize-sizeof(*_obj));
#ifdef _DEBUG
	_obj->_allocator = (allocator_t)a;
#endif	
	return (void*)_obj->data;
}


static void   _free(allocator_t _,void *ptr){
	objpool_t a = (objpool_t)_;	
	if(a->usesystem) return free(ptr);
	obj *_obj = (obj*)((char*)ptr - sizeof(obj));
#ifdef _DEBUG
	assert(_obj->_allocator == (allocator_t)a);
#endif
	chunk *_chunk = a->chunks[_obj->chunkidx];
	uint8_t oldhead = _chunk->head;
	chunk_dealloc(_chunk,_obj);
	if(!oldhead)
		kn_list_pushback(&a->freechunk,(kn_list_node*)_chunk);
}

allocator_t objpool_new(size_t objsize,uint32_t initnum){
	uint32_t chunkcount;
	if(initnum%255 == 0)
		chunkcount = initnum/255;
	else
		chunkcount = initnum/255 + 1;
	if(chunkcount > 0xFFFFFF) return NULL;		
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
			kn_list_pushback(&pool->freechunk,(kn_list_node*)pool->chunks[0]);
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
*/
