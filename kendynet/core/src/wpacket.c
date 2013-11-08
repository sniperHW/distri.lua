#include "wpacket.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "rpacket.h"
#include "common_define.h"
#include "atomic.h"

allocator_t wpacket_allocator = NULL;

wpacket_t wpk_create(uint32_t size,uint8_t is_raw)
{
	size = size_of_pow2(size);
	wpacket_t w = (wpacket_t)ALLOC(wpacket_allocator,sizeof(*w));
	struct packet *base = (struct packet*)w;
	w->factor = size;
	base->raw = is_raw;
	base->buf = buffer_create_and_acquire(NULL,size);
	w->writebuf = buffer_acquire(NULL,base->buf);
	base->begin_pos = 0;
	base->next.next = NULL;
	base->type = MSG_WPACKET;
	if(is_raw)
	{
		w->wpos = 0;
		w->len = 0;
		base->buf->size = 0;
		w->data_size = 0;
	}
	else
	{
		w->wpos = sizeof(*(w->len));
		w->len = (uint32_t*)base->buf->buf;
		*(w->len) = 0;
		base->buf->size = sizeof(*(w->len));
		w->data_size = sizeof(*(w->len));
	}
	return w;
}


static wpacket_t wpk_create_by_wpacket(struct wpacket *_w)
{
	wpacket_t w = (wpacket_t)ALLOC(wpacket_allocator,sizeof(*w));
	w->base.raw = _w->base.raw;
	w->factor = _w->factor;
	w->writebuf = buffer_acquire(NULL,_w->writebuf);
	w->base.begin_pos = _w->base.begin_pos;
	w->base.buf = buffer_acquire(NULL,_w->base.buf);
	w->len = _w->len;//触发拷贝之前len没有作用
	w->wpos = _w->wpos;
	w->base.next.next = NULL;
	w->base.type = MSG_WPACKET;
	w->data_size = _w->data_size;
	return w;
}

static inline wpacket_t wpk_create_by_rpacket(struct rpacket *r)
{
	wpacket_t w = (wpacket_t)ALLOC(wpacket_allocator,sizeof(*w));
	struct packet *base = (struct packet*)w;
	struct packet *other_base = (struct packet*)r;
	base->raw = other_base->raw;
	w->factor = 0;
	w->writebuf = NULL;
	base->begin_pos = other_base->begin_pos;
	base->buf = buffer_acquire(NULL,other_base->buf);
	w->len = 0;//触发拷贝之前len没有作用
	w->wpos = 0;
	base->next.next = NULL;
	base->type = MSG_WPACKET;
	if(base->raw)
		w->data_size = r->len;
	else
		w->data_size = r->len + sizeof(r->len);
	return w;
}

wpacket_t wpk_create_by_other(struct packet *other)
{
	if(other->type == MSG_RPACKET)
		return wpk_create_by_rpacket((struct rpacket*)other);
	else if(other->type == MSG_WPACKET)
		return wpk_create_by_wpacket((struct wpacket*)other);
	else
		return NULL;
}


void wpk_destroy(wpacket_t *w)
{
	buffer_release(&(*w)->base.buf);
	buffer_release(&(*w)->writebuf);
	FREE(wpacket_allocator,*w);
	*w = NULL;
}

