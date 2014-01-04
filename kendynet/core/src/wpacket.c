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
    PACKET_RAW(w) = is_raw;
    PACKET_BUF(w) = buffer_create_and_acquire(NULL,size);
    w->writebuf = buffer_acquire(NULL,PACKET_BUF(w));
    PACKET_BEGINPOS(w)= 0;
    MSG_NEXT(w) = NULL;
    MSG_TYPE(w) = MSG_WPACKET;
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
        w->len = (uint32_t*)PACKET_BUF(w)->buf;
		*(w->len) = 0;
        PACKET_BUF(w)->size = sizeof(*(w->len));
		w->data_size = sizeof(*(w->len));
	}
	return w;
}


static wpacket_t wpk_create_by_wpacket(struct wpacket *_w)
{
	wpacket_t w = (wpacket_t)ALLOC(wpacket_allocator,sizeof(*w));
    PACKET_RAW(w) = PACKET_RAW(_w);
	w->factor = _w->factor;
	w->writebuf = buffer_acquire(NULL,_w->writebuf);
    PACKET_BEGINPOS(w) = PACKET_BEGINPOS(_w);
    PACKET_BUF(w) = buffer_acquire(NULL,PACKET_BUF(_w));
	w->wpos = _w->wpos;
    MSG_NEXT(w) = NULL;
    MSG_TYPE(w) = MSG_WPACKET;
	w->data_size = _w->data_size;
	return w;
}

static inline wpacket_t wpk_create_by_rpacket(struct rpacket *r)
{
	wpacket_t w = (wpacket_t)ALLOC(wpacket_allocator,sizeof(*w));
    PACKET_RAW(w) = PACKET_RAW(r);
	w->factor = 0;
	w->writebuf = NULL;
    PACKET_BEGINPOS(w) = PACKET_BEGINPOS(r);
    PACKET_BUF(w) = buffer_acquire(NULL,PACKET_BUF(r));
	w->len = 0;//触发拷贝之前len没有作用
	w->wpos = 0;
    MSG_NEXT(w) = NULL;
    MSG_TYPE(w) = MSG_WPACKET;
    if(PACKET_RAW(w))
		w->data_size = r->len;
	else
		w->data_size = r->len + sizeof(r->len);
	return w;
}

wpacket_t wpk_create_by_other(struct packet *other)
{
    if(MSG_TYPE(other) == MSG_RPACKET)
		return wpk_create_by_rpacket((struct rpacket*)other);
    else if(MSG_TYPE(other) == MSG_WPACKET)
		return wpk_create_by_wpacket((struct wpacket*)other);
	else
		return NULL;
}


void wpk_destroy(wpacket_t *w)
{
    buffer_release(&PACKET_BUF(*w));
	buffer_release(&(*w)->writebuf);
	FREE(wpacket_allocator,*w);
	*w = NULL;
}

