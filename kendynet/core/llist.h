/*
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
 *单向链表
*/
#ifndef _LLIST_H
#define _LLIST_H


#include "sync.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct lnode
{
    struct lnode *next;
}lnode;


struct llist
{
	int32_t size;
    lnode *head;
    lnode *tail;

};

static inline struct lnode* llist_head(struct llist *l)
{
	return l->head;
}

static inline struct lnode* llist_tail(struct llist *l)
{
	return l->tail;
}


static inline void llist_swap(struct llist *to,struct llist *from)
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

static inline void llist_init(struct llist *l)
{
	l->size = 0;
	l->head = l->tail = NULL;
}

static inline void llist_push_back(struct llist *l,struct lnode *n)
{
    if(n->next) return;
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

static inline void llist_push_front(struct llist *l,struct lnode *n)
{
    if(n->next) return;
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

static inline struct lnode* llist_pop(struct llist *l)
{
	if(0 == l->size)
		return NULL;
    lnode *ret = l->head;
	l->head = l->head->next;
	if(NULL == l->head)
		l->tail = NULL;
	--l->size;
	ret->next = NULL;
	return ret;
}

static inline int32_t llist_is_empty(struct llist *l)
{
	return l->size == 0;
}

static inline int32_t llist_size(struct llist *l)
{
	return l->size;
}

#define LLIST_PUSH_FRONT(L,N) llist_push_front(L,(lnode*)N)

#define LLIST_PUSH_BACK(L,N) llist_push_back(L,(lnode*)N)

#define LLIST_POP(T,L) (T)llist_pop(L)

#define LLIST_IS_EMPTY(L) llist_is_empty(L)


#endif
