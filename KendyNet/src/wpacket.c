#include "wpacket.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "rpacket.h"
#include "kn_common_define.h"
#include "kn_atomic.h"

wpacket_t wpk_create(uint32_t size)
{
	size = size_of_pow2(size);
	if(size < 64) size = 64;
	wpacket_t w = (wpacket_t)calloc(1,sizeof(*w));
	packet_buf(w) = buffer_create(size);
	w->writebuf = buffer_acquire(NULL,packet_buf(w));
	packet_begpos(w)= 0;
	packet_next(w) = NULL;
	packet_type(w) = WPACKET;
	w->wpos = sizeof(*(w->len));
	w->len = (uint32_t*)packet_buf(w)->buf;
	*(w->len) = 0;
	packet_buf(w)->size = sizeof(*(w->len));
	packet_datasize(w) = sizeof(*(w->len));
	return w;
}

wpacket_t wpk_create_by_bin(int8_t *addr,uint32_t size)
{	
	uint32_t datasize = size;
	size = size_of_pow2(size);
	if(size < 64) size = 64;
	wpacket_t w = (wpacket_t)calloc(1,sizeof(*w));
	packet_buf(w) = buffer_create(size);
	w->writebuf = buffer_acquire(NULL,packet_buf(w));
	packet_begpos(w)= 0;
	packet_next(w) = NULL;
	packet_type(w) = WPACKET;
	memcpy(&w->writebuf->buf[0],addr,datasize);		
	w->len = (uint32_t*)packet_buf(w)->buf;
	w->wpos = datasize;
	packet_buf(w)->size = datasize;
	packet_datasize(w) = datasize;	
	return w;	
}

wpacket_t wpk_create_by_wpacket(struct wpacket *_w)
{
	wpacket_t w = (wpacket_t)calloc(1,sizeof(*w));
	w->writebuf = NULL;
	w->len = NULL;
	packet_begpos(w) = packet_begpos(_w);
	packet_buf(w) = buffer_acquire(NULL,packet_buf(_w));
	packet_next(w) = NULL;
	packet_type(w) = WPACKET;
	packet_datasize(w) = packet_datasize(_w);
	return w;
}

wpacket_t wpk_create_by_rpacket(struct rpacket *r)
{
	wpacket_t w = (wpacket_t)calloc(1,sizeof(*w));
	w->writebuf = NULL;
	w->len = NULL;//触发拷贝之前len没有作用
	w->wpos = 0;
	packet_next(w) = NULL;
	packet_type(w) = WPACKET;
	packet_begpos(w) = packet_begpos(r);
	packet_buf(w) = buffer_acquire(NULL,packet_buf(r));
	packet_datasize(w) = kn_ntoh32(r->len) + sizeof(r->len);
	return w;
}

wpacket_t wpk_copy_create(packet_t other){
    if(packet_type(other) == RPACKET)
		return wpk_create_by_rpacket((rpacket_t)other);
    else if(packet_type(other) == WPACKET)
		return wpk_create_by_wpacket((wpacket_t)other);
	return NULL;	
}

void wpk_write_wpk(wpacket_t w,wpacket_t value){
	uint32_t hlen = kn_ntoh32(*value->len) + sizeof(*value->len);
	wpk_write_uint32(w,hlen);	
	uint32_t pos = ((packet_t)value)->begin_pos;
	buffer_t buf = ((packet_t)value)->buf;
	uint32_t sizeremain = hlen; 
	while(sizeremain){
		uint32_t size = buf->size - pos;
		size = size > sizeremain ? sizeremain : size;
		wpk_write(w,&buf->buf[pos],size);
		sizeremain -= size;
		pos = 0;
		buf = buf->next;
	}	
}

