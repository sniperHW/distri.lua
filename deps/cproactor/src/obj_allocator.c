#include "obj_allocator.h"
#include "kn_common_define.h"
#include "lockfree.h"
#include <pthread.h>
struct obj_block
{
	kn_dlist_node node;
	kn_list       freelist;
	lockfree_stack_t que;
	pthread_t       thdid;//·ÖÅäÏß³ÌµÄid
	char   buf[0];
};

struct obj_slot
{
	kn_list_node      node;
	struct obj_block *block;
	char buf[0];
};


struct pth_allocator
{
	lockfree_stack que;
	uint32_t free_block_size;
	kn_dlist free_blocks;
	kn_dlist recy_blocks;
};


struct obj_allocator{
	struct kn_allocator base;
	uint32_t alloc_size;
	uint32_t objsize;
	pthread_key_t pkey;
};

static struct pth_allocator* new_pth(obj_allocator_t allo)
{
	struct pth_allocator *pth = calloc(1,sizeof(*pth));
	kn_dlist_init(&pth->recy_blocks);
	kn_dlist_init(&pth->free_blocks);
	return pth;
}

static inline void __dealloc(obj_allocator_t _allo,struct pth_allocator *pth,struct obj_slot *obj)
{
		kn_list_pushback(&obj->block->freelist,(kn_list_node*)obj);
		uint32_t lsize = kn_list_size(&obj->block->freelist);
		if(unlikely(lsize == 1)){
				kn_dlist_remove((kn_dlist_node*)obj->block);//remove from _alloc->recy_blocks
				kn_dlist_push(&pth->free_blocks,(kn_dlist_node*)obj->block);	
				pth->free_block_size++;
		}
		else if(unlikely(lsize == _allo->alloc_size && pth->free_block_size > 1))
		{
			kn_dlist_remove((kn_dlist_node*)obj->block);//remove from _alloc->free_blocks
			pth->free_block_size--;
			free(obj->block);
		}
}


static inline void* __alloc(obj_allocator_t _allo,struct pth_allocator *pth)
{
	struct obj_block *b = (struct obj_block*)kn_dlist_first(&pth->free_blocks);
	struct obj_slot *obj = (struct obj_slot *)kn_list_pop(&b->freelist);
	if(unlikely(!kn_list_size(&b->freelist)))
	{
		//remove from _alloc->free_blocks and push to _alloc->recy_blocks
		kn_dlist_remove((kn_dlist_node*)b);
		kn_dlist_push(&pth->recy_blocks,(kn_dlist_node*)b);	
	}
	memset(obj->buf,0,_allo->objsize-sizeof(struct obj_slot));
	return (void*)obj->buf;
}

static inline void __expand(obj_allocator_t _allo,struct pth_allocator *pth)
{

	struct obj_block *b = calloc(1,sizeof(*b)+_allo->alloc_size*_allo->objsize);
	b->que = &pth->que;
	b->thdid = pthread_self();
	kn_list_init(&b->freelist);
	uint32_t i = 0;
	for(; i < _allo->alloc_size;++i)
	{
		struct obj_slot *o = (struct obj_slot*)&b->buf[i*_allo->objsize];
		o->block = b;
		kn_list_pushback(&b->freelist,(kn_list_node*)o);
	}
	
	kn_dlist_push(&pth->free_blocks,(kn_dlist_node*)b);	
	++pth->free_block_size;
}

void* obj_alloc(struct kn_allocator *allo,int32_t size)
{
	obj_allocator_t _allo = (obj_allocator_t)allo;
	struct pth_allocator *pth = (struct pth_allocator*)pthread_getspecific(_allo->pkey);
	if(unlikely(!pth))
	{
		pth = new_pth(_allo);
		pthread_setspecific(_allo->pkey,pth);
		
	}
	if(unlikely(kn_dlist_empty(&pth->free_blocks)))
	{
		kn_list_node *n;
		while((n = lfstack_pop(&pth->que)) != NULL)
			__dealloc(_allo,pth,(struct obj_slot*)n);
				
	}else
		return __alloc(_allo,pth);

	if(unlikely(kn_dlist_empty(&pth->free_blocks)))
		__expand(_allo,pth);
	return __alloc(_allo,pth);
}



void obj_dealloc(struct kn_allocator *allo ,void *ptr)
{
	obj_allocator_t _allo = (obj_allocator_t)allo;
	struct obj_slot *obj = (struct obj_slot*)((char*)ptr - sizeof(struct obj_slot));	
	if(obj->block->thdid == pthread_self()){

		struct pth_allocator *pth = (struct pth_allocator*)pthread_getspecific(_allo->pkey);;
		if(unlikely(!pth))
			abort();
		__dealloc(_allo,pth,obj);
	}
	else
	{
		lfstack_push(obj->block->que,(kn_list_node*)obj);
	}
}
	
kn_allocator_t new_obj_allocator(uint32_t objsize,uint32_t initsize)
{
	obj_allocator_t allo = calloc(1,sizeof(*allo));
	objsize += sizeof(struct obj_slot);
    objsize = size_of_pow2(objsize);
    if(objsize < 64) objsize = 64;
	initsize = size_of_pow2(initsize);
	if(initsize < 1024) initsize = 1024;
	allo->alloc_size = initsize;
	allo->objsize = objsize;
    pthread_key_create(&allo->pkey,NULL);
	((kn_allocator_t)allo)->_alloc = obj_alloc;
	((kn_allocator_t)allo)->_dealloc = obj_dealloc;
	((kn_allocator_t)allo)->_destroy = NULL;
	return (kn_allocator_t)allo;

}
