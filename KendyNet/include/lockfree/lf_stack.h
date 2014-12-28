#ifndef  _LF_STACK_H
#define _LF_STACK_H
#include "kn_list.h"
#include "kn_atomic.h"

typedef struct lockfree_stack
{
	kn_list_node * volatile head;
}lockfree_stack,*lockfree_stack_t;

static  inline void lfstack_push(lockfree_stack_t ls,kn_list_node *n)
{
	for( ; ;){	
		kn_list_node *lhead = ls->head;
		n->next = lhead;
		if(COMPARE_AND_SWAP(&ls->head,lhead,n))//if head unchange,set n to be the new head
			break;
	}
}

static  inline kn_list_node* lfstack_pop(lockfree_stack_t ls)
{
	for( ; ;){	
		kn_list_node *lhead = ls->head;
		if(!lhead) return NULL;
		kn_list_node *next = lhead->next;				
		if(COMPARE_AND_SWAP(&ls->head,lhead,next))
		{
			lhead->next = NULL;
			return lhead;
		}
	}
}

#endif
