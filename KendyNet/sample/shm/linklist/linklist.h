#include <stdint.h>


#define SHM_LNKLIST(NAME,TYPE,POOLSIZE)                                                                            	\
struct NAME##node{                                                                                                                  	\
	int32_t next;                                                                                                                   	\
	TYPE    value;                                                                                                                  	\
};												\
												\
typedef struct {											\
	int32_t head;										\
	int32_t tail;										\
	int32_t size;										\
	int32_t  head_mempool;									\
	struct NAME##node memblock[POOLSIZE];						\
}NAME;												\
												\
void NAME##_init(NAME *l){									\
	l->head = l->tail = -1;									\
	l->size = 0;										\
	int i = 0;										\
	for(; i < POOLSIZE; ++i){									\
		l->memblock[i].next = i + 1;							\
	}											\
	l->memblock[i].next = -1;								\
	l->head_mempool = 0; 									\
}												\
												\
int NAME##_push(NAME *l,int value){								\
	if(l->size == POOLSIZE || l->head_mempool == -1)					\
		return -1;									\
	struct NAME##node *_node = &l->memblock[l->head_mempool];				\
	_node->value = value;									\
	if(l->head == -1){									\
		l->tail = l->head = l->head_mempool;						\
	}else{											\
		struct NAME##node *last_node = &l->memblock[l->tail];				\
		l->tail = last_node->next = l->head_mempool;					\
	}											\
	l->head_mempool = _node->next;							\
	_node->next = -1;									\
	++l->size;										\
	return 0;										\
}												\
												\
int NAME##_pop(NAME *l, TYPE *ret){								\
	if(l->size == 0 || l->head == -1 || !ret) 							\
		return -1;									\
	struct NAME##node *_node = &l->memblock[l->head];					\
	*ret = _node->value;									\
	l->head = _node->next;									\
	if(l->head == -1) l->tail = -1;								\
	_node->next = l->head_mempool;							\
	l->head_mempool = ((char*)_node -  (char*)l->memblock)/sizeof(*_node);			\
	--l->size;										\
	return 0;										\
}

