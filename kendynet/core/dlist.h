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

//双向链表
#ifndef _DLIST_H
#define _DLIST_H

struct dnode
{
    struct dnode *pre;
    struct dnode *next;
};

struct dlist
{
    struct   dnode head;
    struct   dnode tail;
};


static inline int32_t dlist_empty(struct dlist *dl)
{
    return dl->head.next == &dl->tail ? 1:0;
}


static inline struct dnode *dlist_first(struct dlist *dl)
{
    if(dlist_empty(dl))
		return NULL;
	return dl->head.next;
}

static inline struct dnode *dlist_last(struct dlist *dl)
{
    if(dlist_empty(dl))
		return NULL;
	return dl->tail.pre;
}


static inline int32_t dlist_remove(struct dnode *dln)
{
    if(!dln->pre && !dln->next) return -1;
    dln->pre->next = dln->next;
	dln->next->pre = dln->pre;
	dln->pre = dln->next = NULL;
	return 0;
}

static inline struct dnode *dlist_pop(struct dlist *dl)
{
    if(dlist_empty(dl))
		return NULL;
	else
	{
        struct dnode *n = dl->head.next;
        dlist_remove(n);
		return n;
	}
}

static inline int32_t dlist_push(struct dlist *dl,struct dnode *dln)
{
	if(dln->pre || dln->next) return -1;
	dl->tail.pre->next = dln;
	dln->pre = dl->tail.pre;
	dl->tail.pre = dln;
	dln->next = &dl->tail;
	return 0;
}

static inline void dlist_init(struct dlist *dl)
{
	dl->head.pre = dl->tail.next = NULL;
	dl->head.next = &dl->tail;
	dl->tail.pre = &dl->head;
}

//if the dblnk_check return 1,dln will be remove
typedef int8_t (*dblnk_check)(struct dnode *dln,void *);

static inline void dlist_check_remove(struct dlist *dl,dblnk_check _check,void *ud)
{
    if(dlist_empty(dl)) return;

    struct dnode *dln = dl->head.next;
    while(dln != &dl->tail)
    {
        struct dnode *tmp = dln;
        dln = dln->next;
        if(_check(tmp,ud) == 1){
            //remove
            dlist_remove(tmp);
        }
    }
}

static inline void dlist_move(struct dlist *dl1,struct dlist *dl2)
{
    if(dlist_empty(dl2)) return;
	dl2->head.next->pre = dl1->tail.pre;
	dl2->tail.pre->next = &dl1->tail;
	dl1->tail.pre->next = dl2->head.next;
	dl1->tail.pre = dl2->tail.next;
    dlist_init(dl2);
}
#endif
