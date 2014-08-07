#include "wpacket.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "rpacket.h"
#include "kn_common_define.h"
#include "kn_atomic.h"

kn_allocator_t wpacket_allocator = NULL;

wpacket_t wpk_create(uint32_t size,uint8_t is_raw)
{
	size = size_of_pow2(size);
    if(size < 64) size = 64;
    wpacket_t w = (wpacket_t)ALLOC(wpacket_allocator,sizeof(*w));
    PACKET_RAW(w) = is_raw;
    PACKET_BUF(w) = buffer_create(size);
    w->writebuf = buffer_acquire(NULL,PACKET_BUF(w));
    PACKET_BEGINPOS(w)= 0;
    MSG_NEXT(w) = NULL;
    MSG_TYPE(w) = MSG_WPACKET;
	if(is_raw)
	{
		w->wpos = 0;
        w->len = NULL;
        PACKET_BUF(w)->size = 0;
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


/*wpacket_t wpk_create_by_buffer(buffer_t buffer,uint32_t begpos,uint32_t len,uint32_t is_raw)
{
    wpacket_t w = (wpacket_t)ALLOC(wpacket_allocator,sizeof(*w));
    w->data_size = len;
    MSG_NEXT(w) = NULL;
    MSG_TYPE(w) = MSG_WPACKET;
    PACKET_BEGINPOS(w) = begpos;
    PACKET_BUF(w) = buffer_acquire(NULL,buffer);
    PACKET_RAW(w) = is_raw;
    if(is_raw)
        w->len = NULL;
    else
        w->len = (uint32_t*)&buffer->buf[begpos];
    w->writebuf = NULL;
    w->len = NULL;
    return w;
}*/


wpacket_t wpk_create_by_wpacket(struct wpacket *_w)
{
	wpacket_t w = (wpacket_t)ALLOC(wpacket_allocator,sizeof(*w));
    PACKET_RAW(w) = PACKET_RAW(_w);
    w->writebuf = NULL;
    w->len = NULL;
    PACKET_BEGINPOS(w) = PACKET_BEGINPOS(_w);
    PACKET_BUF(w) = buffer_acquire(NULL,PACKET_BUF(_w));
    MSG_NEXT(w) = NULL;
    MSG_TYPE(w) = MSG_WPACKET;
	w->data_size = _w->data_size;
	return w;
}

wpacket_t wpk_create_by_rpacket(struct rpacket *r/*,uint32_t dropsize*/)
{
    //if(!dropsize){
        wpacket_t w = (wpacket_t)ALLOC(wpacket_allocator,sizeof(*w));
        PACKET_RAW(w) = PACKET_RAW(r);
        w->writebuf = NULL;
        w->len = NULL;//触发拷贝之前len没有作用
        w->wpos = 0;
        MSG_NEXT(w) = NULL;
        MSG_TYPE(w) = MSG_WPACKET;

        PACKET_BEGINPOS(w) = PACKET_BEGINPOS(r);
        PACKET_BUF(w) = buffer_acquire(NULL,PACKET_BUF(r));
        if(PACKET_RAW(w))
            w->data_size = r->len;
        else
            w->data_size = r->len + sizeof(r->len);
        return w;
    /*}else
    {
        uint32_t _dropsize = dropsize;
        buffer_t buffer = rpk_readbuf(r);
        uint32_t index  = rpk_rpos(r);
        while(_dropsize>0)
        {
             uint32_t move = buffer->size - index;
             if(move > _dropsize) move = _dropsize;
             index += move;
             _dropsize -= move;
             if(index == buffer->size){
                 buffer = buffer->next;
                 index = 0;
             }
             assert(buffer);
         }
         return wpk_create_by_buffer(buffer,index,rpk_data_remain(r)-dropsize,PACKET_RAW(r));
    }*/
}

void wpk_destroy(wpacket_t w)
{
    buffer_release(&PACKET_BUF(w));
	buffer_release(&w->writebuf);
	FREE(wpacket_allocator,w);
}

