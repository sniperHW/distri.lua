#ifndef _LINK_LIST_H
#define _LINK_LIST_H


#include "sync.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct list_node
{
	struct list_node *next;
}list_node;

#define LIST_NODE list_node node;

struct link_list
{
	int32_t size;
	list_node *head;
	list_node *tail;
	
};

static inline struct link_list *create_link_list()
{
	struct link_list *l = calloc(1,sizeof(*l));
	return l;
}

static inline void destroy_link_list(struct link_list **l)
{
	free(*l);
	*l = NULL;
}

static inline struct list_node* link_list_head(struct link_list *l)
{
	return l->head;
}

static inline struct list_node* link_list_tail(struct link_list *l)
{
	return l->tail;
}


static inline void link_list_swap(struct link_list *to,struct link_list *from)
{
	if(from->head && from->tail)
	{
		if(to->tail)
			to->tail->next = from->head;
		else
			to->head = from->head;
		to->tail = from ->tail;
		from->head = from->tail = NULL;
		to->size += from->size;
		from->size = 0;
	}
}

static inline void link_list_clear(struct link_list *l)
{
	l->size = 0;
	l->head = l->tail = NULL;
}

static inline void link_list_push_back(struct link_list *l,struct list_node *n)
{
	if(n->next)
	{
		printf("push error\n");
		exit(0);
		return;
	}
	n->next = NULL;
	if(0 == l->size)
		l->head = l->tail = n;
	else
	{
		l->tail->next = n;
		l->tail = n;
	}
	++l->size;
}

static inline void link_list_push_front(struct link_list *l,struct list_node *n)
{
	if(n->next)
	{
		printf("push error\n");
		exit(0);
		return;
	}
	n->next = NULL;
	if(0 == l->size)
		l->head = l->tail = n;
	else
	{
		n->next = l->head;
		l->head = n;
	}
	++l->size;

}

static inline struct list_node* link_list_pop(struct link_list *l)
{
	if(0 == l->size)
		return NULL;
	list_node *ret = l->head;
	l->head = l->head->next;
	if(NULL == l->head)
		l->tail = NULL;
	--l->size;
	ret->next = NULL;
	return ret;
}

static inline int32_t link_list_is_empty(struct link_list *l)
{
	return l->size == 0;
}

static inline int32_t link_list_size(struct link_list *l)
{
	return l->size;
}

#define LINK_LIST_PUSH_FRONT(L,N) link_list_push_front(L,(list_node*)N)

#define LINK_LIST_PUSH_BACK(L,N) link_list_push_back(L,(list_node*)N)

#define LINK_LIST_POP(T,L) (T)link_list_pop(L)

#define LINK_LIST_IS_EMPTY(L) link_list_is_empty(L)

#define LINK_LIST_CREATE() create_link_list()

#define LINK_LIST_DESTROY(L) destroy_link_list(L)

#define LINK_LIST_CLEAR(L) link_list_clear(L)

#endif
