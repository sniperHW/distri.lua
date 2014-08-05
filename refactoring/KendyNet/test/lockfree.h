#ifndef _LOCKFREE_H
#define _LOCKFREE_H

#include "kn_list.h"
#include "kn_atomic.h"

typedef struct lockfree_stack
{
	kn_list_node *head;
}lockfree_stack,*lockfree_stack_t;

static inline void lfstack_push(lockfree_stack_t ls,kn_list_node *n)
{
		for( ; ;){
			kn_list_node *lhead = ls->head;
			n->next = lhead;
			if(COMPARE_AND_SWAP(&ls->head,lhead,n))
				break;
		}
}

static inline kn_list_node* lfstack_pop(lockfree_stack_t ls)
{
		for( ; ;){
			kn_list_node *lhead = ls->head;
			if(!lhead) return NULL;
			kn_list_node *ret = ls->head;				
			if(COMPARE_AND_SWAP(&ls->head,lhead,ls->head->next))
			{
				ret->next = NULL;
				return ret;
			}
		}
}

#endif
