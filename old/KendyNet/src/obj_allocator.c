/*
暂时废弃

#include "obj_allocator.h"
#include "kn_common_define.h"
#include "lockfree.h"
#include <pthread.h>
#include "hash_map.c"
#include "common_hash_function.h"
#include "spinlock.h"

//用于回收其它线程释放的内存
struct recyque{
	uint8_t     ownerdead; 
	kn_list     que;
	spinlock_t  lock;
};

struct obj_block
{
	kn_dlist_node     node;
	kn_list   		  freelist;
	struct recyque*   que;
	pthread_t         thdid;         //分配线程的id
	uint32_t          reverse_size;   
	char              buf[0];
};

struct obj_slot
{
	kn_list_node      node;
	struct obj_block *block;
	char buf[0];
};

struct obj_allocator;
struct pth_allocator
{
	struct recyque*que;
	struct obj_allocator *allo;
	uint32_t       free_block_size;
	kn_dlist       free_blocks;
	kn_dlist       recy_blocks;
	uint32_t       free_memsize;
	hash_map_t     local_recycache;
};


struct obj_allocator{
	struct kn_allocator base;
	uint32_t            alloc_size;
	uint32_t            objsize;
	uint32_t            reserve_size;
	pthread_key_t       pkey;
};

struct recylist{
	kn_list_node      node;
	kn_list           list;
};

static void revert(struct recyque *que,struct recylist *l){
	spin_lock(que->lock);
	if(unlikely(que->ownerdead)){
		//如果归属线程已经停止，则没有线程会回收内存，直接在这里处理
		struct obj_slot *obj;
		while((obj = (struct obj_slot*)kn_list_pop(&l->list)) != NULL)
			kn_list_pushback(&obj->block->freelist,(kn_list_node*)obj);
		if(kn_list_size(&obj->block->freelist) == obj->block->reverse_size){
			//收满一个block,直接释放整个block
			free(obj->block);
		}	
		free(l);	
	}else{
		kn_list_pushback(&que->que,(kn_list_node*)l);
	}
	spin_unlock(que->lock);
}

static void getall(struct recyque *que,kn_list *l){
	spin_lock(que->lock);
	kn_list_swap(l,&que->que);
	spin_unlock(que->lock);
}

static uint64_t _hash_func(void *key){
	pthread_t k = *(pthread_t*)key;
	return burtle_hash((uint8_t*)&k,sizeof(k),1);
}

static int32_t  _hash_key_eq(void *key1,void *key2){
	return (*(pthread_t*)key1) == (*(pthread_t*)key2) ? 0:-1;	
}

static struct pth_allocator* new_pth(obj_allocator_t allo)
{
	struct pth_allocator *pth = calloc(1,sizeof(*pth));
	kn_dlist_init(&pth->recy_blocks);
	kn_dlist_init(&pth->free_blocks);
	pth->local_recycache = hash_map_create(64,sizeof(void*),sizeof(void*),_hash_func,_hash_key_eq);
	pth->que = calloc(1,sizeof(*pth->que));
	pth->allo = allo;
	kn_list_init(&pth->que->que);
	pth->que->lock = spin_create();
	return pth;
}

static inline void __dealloc(struct pth_allocator *pth,struct obj_slot *obj)
{
		obj_allocator_t _allo = pth->allo;
		kn_list_pushback(&obj->block->freelist,(kn_list_node*)obj);
		uint32_t lsize = kn_list_size(&obj->block->freelist);
		pth->free_memsize += _allo->objsize;
		
		if(likely(lsize > 1 && lsize < _allo->alloc_size)) return;
		
		if(lsize == 1){
			kn_dlist_remove((kn_dlist_node*)obj->block);//remove from _alloc->recy_blocks
			kn_dlist_push(&pth->free_blocks,(kn_dlist_node*)obj->block);	
			pth->free_block_size++;
			return;
		}
		
		if(lsize == _allo->alloc_size && pth->free_memsize > _allo->reserve_size)
		{
			pth->free_memsize -= _allo->alloc_size*_allo->objsize;
			kn_dlist_remove((kn_dlist_node*)obj->block);//remove from _alloc->free_blocks
			pth->free_block_size--;
			free(obj->block);
		}
}

static inline void* __alloc(struct pth_allocator *pth)
{
	obj_allocator_t _allo = pth->allo;
	struct obj_block *b = (struct obj_block*)kn_dlist_first(&pth->free_blocks);
	struct obj_slot *obj = (struct obj_slot *)kn_list_pop(&b->freelist);
	if(unlikely(!kn_list_size(&b->freelist)))
	{
		//remove from _alloc->free_blocks and push to _alloc->recy_blocks
		kn_dlist_remove((kn_dlist_node*)b);
		kn_dlist_push(&pth->recy_blocks,(kn_dlist_node*)b);	
	}
	pth->free_memsize -= _allo->objsize;
	memset(obj->buf,0,_allo->objsize-sizeof(struct obj_slot));
	return (void*)obj->buf;
}


static __thread pthread_t threadid = 0;

static inline void __expand(struct pth_allocator *pth)
{
	obj_allocator_t _allo = pth->allo;
	struct obj_block *b = calloc(1,sizeof(*b)+_allo->alloc_size*_allo->objsize);
	b->que = pth->que;
	if(unlikely(!threadid)) threadid = pthread_self();
	b->thdid = threadid;
	kn_list_init(&b->freelist);
	uint32_t i = 0;
	for(; i < _allo->alloc_size;++i)
	{
		struct obj_slot *o = (struct obj_slot*)&b->buf[i*_allo->objsize];
		o->block = b;
		kn_list_pushback(&b->freelist,(kn_list_node*)o);
	}
	b->reverse_size = _allo->alloc_size;
	pth->free_memsize += _allo->alloc_size*_allo->objsize;
	kn_dlist_push(&pth->free_blocks,(kn_dlist_node*)b);	
	++pth->free_block_size;
}

void* obj_alloc(struct kn_allocator *allo,int32_t _)
{
	(void)_;
	obj_allocator_t _allo = (obj_allocator_t)allo;
	struct pth_allocator *pth = (struct pth_allocator*)pthread_getspecific(_allo->pkey);
	if(unlikely(!pth))
	{
		pth = new_pth(_allo);
		pthread_setspecific(_allo->pkey,pth);
	}
	if(unlikely(kn_dlist_empty(&pth->free_blocks)))
	{
		kn_list tmp;kn_list_init(&tmp);
		getall(pth->que,&tmp);
		if(kn_list_size(&tmp) > 0){
			struct recylist *l = (struct recylist*)kn_list_pop(&tmp);
			do{
				struct obj_slot *obj;
				while((obj = (struct obj_slot*)kn_list_pop(&l->list)) != NULL){
					__dealloc(pth,obj);
				}
				free(l);
				l = (struct recylist*)kn_list_pop(&tmp);				
			}while(l);
			
		}else
			__expand(pth);		
	}
	return __alloc(pth);
}

void obj_dealloc(struct kn_allocator *allo ,void *ptr)
{
	obj_allocator_t _allo = (obj_allocator_t)allo;
	struct obj_slot *obj = (struct obj_slot*)((char*)ptr - sizeof(struct obj_slot));
	if(unlikely(!threadid)) threadid = pthread_self();	
	if(obj->block->thdid == threadid){
		struct pth_allocator *pth = (struct pth_allocator*)pthread_getspecific(_allo->pkey);;
		if(unlikely(!pth))
			abort();
		__dealloc(pth,obj);
	}
	else{
		struct pth_allocator *pth = (struct pth_allocator*)pthread_getspecific(_allo->pkey);
		if(unlikely(!pth))
		{
			pth = new_pth(_allo);
			pthread_setspecific(_allo->pkey,pth);
		}
		hash_map_iter it = hash_map_find(pth->local_recycache,(void*)&obj->block->thdid); 
		hash_map_iter end = hash_map_end(pth->local_recycache);
		struct recylist *l;
		if(IT_EQ(it,end)){
			l = calloc(1,sizeof(*l));
			kn_list_init(&l->list);
			hash_map_insert(pth->local_recycache,(void*)&obj->block->thdid,(void*)&l);
		}else{
			l = (struct recylist*)IT_GET_VAL(void*,it);
		}
		kn_list_pushback(&l->list,(kn_list_node*)obj);
		if(kn_list_size(&l->list) > 64){
			hash_map_erase(pth->local_recycache,it);
			revert(obj->block->que,l);
		}
	}
}

static void  delete_pth(void *arg)
{
	if(!arg) return;
	//处理block
	struct pth_allocator *pth = arg;
	spin_lock(pth->que->lock);
	pth->que->ownerdead = 1;
	kn_list tmp;kn_list_init(&tmp);
	kn_list_swap(&tmp,&pth->que->que);
	if(kn_list_size(&tmp) > 0){
		struct recylist *l = (struct recylist*)kn_list_pop(&tmp);
		do{
			struct obj_slot *obj;
			while((obj = (struct obj_slot*)kn_list_pop(&l->list)) != NULL){
				__dealloc(pth,obj);
			}
			free(l);
			l = (struct recylist*)kn_list_pop(&tmp);				
		}while(l);
		
	}	
	{
		struct obj_block *blk = (struct obj_block*)kn_dlist_pop(&pth->free_blocks);
		if(blk){
			do{
				free(blk);
				blk = (struct obj_block*)kn_dlist_pop(&pth->free_blocks);
			}while(blk);
		}
	}
	{
		struct obj_block *blk = (struct obj_block*)kn_dlist_pop(&pth->recy_blocks);
		if(blk){
			do{
				free(blk);
				blk = (struct obj_block*)kn_dlist_pop(&pth->recy_blocks);
			}while(blk);
		}
	}
	spin_unlock(pth->que->lock);
	//将	local_recycache中的内存直接返回给分配者
	hash_map_iter it = hash_map_begin(pth->local_recycache);
	hash_map_iter end = hash_map_end(pth->local_recycache);
	for( ; !IT_EQ(it,end); IT_NEXT(it)){
		struct recylist *l = (struct recylist*)IT_GET_VAL(void*,it);
		struct obj_slot *obj = (struct obj_slot*)kn_list_head(&l->list);
		revert(obj->block->que,l);	
	}
	hash_map_destroy(&pth->local_recycache);
}
	
kn_allocator_t new_obj_allocator(uint32_t objsize)
{
	obj_allocator_t allo = calloc(1,sizeof(*allo));
	objsize += sizeof(struct obj_slot);
    objsize = size_of_pow2(objsize);
    if(objsize < 64) objsize = 64;
	if(objsize < 256) allo->alloc_size = 8192/objsize;
	else allo->alloc_size = 32;
	
	if(objsize < 256)
		allo->reserve_size = (1024*1024*64)/objsize;
	else
		allo->reserve_size = objsize * allo->alloc_size * 32;
	allo->objsize = objsize;
    pthread_key_create(&allo->pkey,delete_pth);
	((kn_allocator_t)allo)->_alloc = obj_alloc;
	((kn_allocator_t)allo)->_dealloc = obj_dealloc;
	((kn_allocator_t)allo)->_destroy = NULL;
	return (kn_allocator_t)allo;

}*/
