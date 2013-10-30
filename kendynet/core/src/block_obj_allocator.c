#include "block_obj_allocator.h"
#include <pthread.h>
#include "link_list.h"
#include <stdint.h>
#include <assert.h>
#include "spinlock.h"
#include <stdlib.h>
#include "allocator.h"
#include "double_link.h"

struct memory_block
{
	list_node next;
	void *mem;
};

/*
*  内存块链表
*/
struct mem_list{
	struct double_link_node dnode;
	uint32_t  init_size;
	struct link_list l;
};

/*
 * 每线程一个的分配器
*/
struct thread_allocator
{
	list_node next;
	block_obj_allocator_t central_allocator;
	struct double_link _free_list;
	struct double_link _empty_list;
	uint32_t   free_size;
};

struct block_obj_allocator
{
	IMPLEMEMT(allocator);
	pthread_key_t t_key;
	struct double_link _free_list;
	struct link_list *_memory_blocks;
	spinlock_t mtx;
	struct link_list *_thread_allocators;
	uint32_t obj_size;
};

static void init_mem_block(struct mem_list *m,struct memory_block *mb,uint32_t obj_size)
{
	int32_t i = 0;
	for( ; i < m->init_size; ++i)
	{
		list_node *l = (list_node*)(((uint8_t*)mb->mem)+(i*obj_size));
		LINK_LIST_PUSH_BACK(&m->l,l);
	}
}

static struct mem_list *create_memlist(uint32_t size)
{ 	
	uint32_t init_size = DEFAULT_BLOCK_SIZE/size;
	struct mem_list *m = (struct mem_list*)calloc(1,sizeof(*m));
	m->init_size = init_size;
	return m;	
}

/*
 *从中心分配器获得一个memlist,如果中心分配器没用可用的memlist,则创建一个新的memory_block并通过mb返回给调用者 
*/ 
static inline struct mem_list *central_get_memlist(block_obj_allocator_t central,struct memory_block **mb)
{
	
	struct mem_list *m;
	if(central->mtx)spin_lock(central->mtx);
	m = (struct mem_list*)double_link_pop(&central->_free_list);
	if(!m)
	{
		struct memory_block *_mb = (struct memory_block *)calloc(1,sizeof(*_mb));
		printf("alloc memory_block\n");
		assert(_mb);
		_mb->mem = calloc(1,DEFAULT_BLOCK_SIZE);
		assert(_mb->mem);
		LINK_LIST_PUSH_BACK(central->_memory_blocks,_mb);
		*mb = _mb;
	}
	if(central->mtx)spin_unlock(central->mtx);
	return m; 
}

static inline void give_back_to_central(block_obj_allocator_t central,struct mem_list *m)
{
	if(central->mtx)spin_lock(central->mtx);
	double_link_push(&central->_free_list,(struct double_link_node*)m);
	if(central->mtx)spin_unlock(central->mtx);
}

static inline void *thread_allocator_alloc(struct thread_allocator *a)
{
	void *ptr;
	struct mem_list *m;
	if(!a->free_size)
	{
		//thread cache不够内存了，从central获取
		struct memory_block *mb;
		m = central_get_memlist(a->central_allocator,&mb);
		if(!m)
		{
			m = (struct mem_list *)double_link_pop(&a->_empty_list);
			if(!m)
				m = create_memlist(a->central_allocator->obj_size);
			init_mem_block(m,mb,a->central_allocator->obj_size);
		}	
		double_link_push(&a->_free_list,(struct double_link_node*)m);
		a->free_size += m->l.size;
	}
	else
		m = (struct mem_list*)double_link_first(&a->_free_list);
		
	ptr = LINK_LIST_POP(void*,&m->l);
	--a->free_size;
	assert(ptr);
	if(!m->l.size)
	{
		double_link_remove((struct double_link_node*)m);
		double_link_push(&a->_empty_list,(struct double_link_node*)m);
	}
	return ptr;		
}

static inline void thread_allocator_dealloc(struct thread_allocator *a,void *ptr)
{
	struct mem_list *m = NULL;
	struct double_link_node *dl = double_link_last(&a->_free_list);
	if(dl)
	{
		while(dl != &a->_free_list.head)
		{
			m = (struct mem_list*)dl;
			if(m->l.size < m->init_size)
				break;
			m = NULL;
			dl = dl->pre;	
		}
	}
	
	if(!m)
	{
		m = (struct mem_list *)double_link_pop(&a->_empty_list);
		if(!m)
			m = create_memlist(a->central_allocator->obj_size);
		double_link_push(&a->_free_list,(struct double_link_node*)m);
	}
	LINK_LIST_PUSH_BACK(&m->l,ptr);
	++a->free_size;
	if(m->l.size == m->init_size)
	{
		if(a->free_size >= m->init_size*2)
		{
			a->free_size -= m->l.size;
			double_link_remove((struct double_link_node*)m);
			give_back_to_central(a->central_allocator,m);
		}	
	}
}

static void release_memlist(struct double_link *mlist)
{
	struct double_link_node *n;
	while((n = double_link_pop(mlist)) != NULL)
		free(n);
}

void destroy_thread_allocator(struct thread_allocator *a)
{
	release_memlist(&a->_free_list);
	release_memlist(&a->_empty_list);
	free(a);
}

struct thread_allocator *create_thread_allocator(block_obj_allocator_t ba)
{
	struct thread_allocator *a = (struct thread_allocator*)calloc(1,sizeof(*a));
	if(a)
	{
		a->central_allocator = ba;
		double_link_clear(&a->_free_list);
		double_link_clear(&a->_empty_list);
		if(ba->mtx)spin_lock(ba->mtx);
		LINK_LIST_PUSH_BACK(ba->_thread_allocators,a);
		if(ba->mtx)spin_unlock(ba->mtx);
	}
	return a;
}

static void* block_obj_al_alloc(struct allocator *a,int32_t size)
{
	block_obj_allocator_t ba = (block_obj_allocator_t)a;
	struct thread_allocator *ta;
	if(ba->mtx)
	{
		ta = (struct thread_allocator*)pthread_getspecific(ba->t_key);
		if(!ta)
		{
			ta = create_thread_allocator(ba);
			pthread_setspecific(ba->t_key,(void*)ta);
		}
	}
	else
	{
		ta = (struct thread_allocator*)link_list_head(ba->_thread_allocators);
		if(!ta)
			ta = create_thread_allocator(ba);
	}
	return thread_allocator_alloc(ta);
}

static void  block_obj_al_dealloc(struct allocator*a, void *ptr)
{
	block_obj_allocator_t ba = (block_obj_allocator_t)a;
	struct thread_allocator *ta;
	if(ba->mtx)
	{
		ta = (struct thread_allocator*)pthread_getspecific(ba->t_key);
		if(!ta)
		{
			ta = create_thread_allocator(ba);
			pthread_setspecific(ba->t_key,(void*)ta);
		}
	}
	else
		ta = (struct thread_allocator*)link_list_head(ba->_thread_allocators);
	assert(ta);
	thread_allocator_dealloc(ta,ptr);
}

static void destroy_block_obj_al(struct allocator **a)
{
	block_obj_allocator_t ba = (block_obj_allocator_t)*a;
    //销毁所有的thread_cache
	struct thread_allocator *ta;
	while((ta = LINK_LIST_POP(struct thread_allocator *,ba->_thread_allocators))!=NULL)
		destroy_thread_allocator(ta);
	LINK_LIST_DESTROY(&ba->_thread_allocators);
    
	struct memory_block *mb;
	while((mb = LINK_LIST_POP(struct memory_block*,ba->_memory_blocks))!=NULL)
	{
		free(mb->mem);
		free(mb);
	}
	LINK_LIST_DESTROY(&ba->_memory_blocks);			
	release_memlist(&ba->_free_list);	
	if(ba->mtx)
	{
		spin_destroy(&(ba->mtx));
		pthread_key_delete(ba->t_key);
	}
	free(ba);
	*a = NULL;	
}

#include "buffer.h"

block_obj_allocator_t create_block_obj_allocator(uint8_t mt,uint32_t obj_size)
{
	if(obj_size < sizeof(void*))
		obj_size = sizeof(void*);
	uint8_t k = GetK(obj_size);
	obj_size = 1 << k;
	block_obj_allocator_t ba = (block_obj_allocator_t)calloc(1,sizeof(*ba));
	if(mt)
	{
		ba->mtx = spin_create();
		pthread_key_create(&ba->t_key,0);
	}
	ba->_thread_allocators = LINK_LIST_CREATE();
	ba->_memory_blocks = LINK_LIST_CREATE();
	double_link_clear(&ba->_free_list);
	ba->obj_size = obj_size;
	ba->super_class._alloc = block_obj_al_alloc;
	ba->super_class._dealloc = block_obj_al_dealloc;
	ba->super_class._destroy = destroy_block_obj_al;
	return ba;
}
