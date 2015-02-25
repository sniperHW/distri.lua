#include "kendynet.h"
#include "packet.h"
#include "wpacket.h"
#include "rpacket.h"
#include "rawpacket.h"

allocator_t g_rawpk_allocator = NULL;

static inline void rawpacket_destroy(rawpacket_t p)
{
	//释放所有对buffer_t的引用
	buffer_release(packet_buf(p));
	FREE(g_rawpk_allocator,p);
	//free(p);
}

void rpk_destroy(rpacket_t r)
{
	//释放所有对buffer_t的引用
	buffer_release(packet_buf(r));
	//buffer_release(r->readbuf);
	buffer_release(r->binbuf);
	FREE(g_rpk_allocator,r);
	//free(r);
}

void wpk_destroy(wpacket_t w)
{
	buffer_release(packet_buf(w));
	//buffer_release(w->writebuf);
	FREE(g_wpk_allocator,w);
	//free(w);
}

void _destroy_packet(packet_t p){
	if(packet_type(p) == RPACKET) rpk_destroy((rpacket_t)p);
	else if(packet_type(p) == WPACKET) wpk_destroy((wpacket_t)p);
	else if(packet_type(p) == RAWPACKET) rawpacket_destroy((rawpacket_t)p);
}
\
static packet_t rawpacket_clone(packet_t other);

rawpacket_t rawpacket_create1(buffer_t b,uint32_t pos,uint32_t len){
	if(!b) return NULL;
	rawpacket_t p = (rawpacket_t)CALLOC(g_rawpk_allocator,1,sizeof(*p));//calloc(1,sizeof(*p));
	packet_buf(p) = buffer_acquire(NULL,b);
	packet_begpos(p) = pos;
	packet_type(p) = RAWPACKET;
	packet_datasize(p) = len;
	packet_clone(p) = rawpacket_clone;
	packet_makeforwrite(p) = rawpacket_clone;
	packet_makeforread(p) = rawpacket_clone;	
	return p;
}

rawpacket_t rawpacket_create2(void *ptr,uint32_t len){
	if(!ptr) return NULL;
    	uint32_t size = size_of_pow2(len);
    	if(size < 64) size = 64;
    	buffer_t tmp = buffer_create(size);
    	memcpy(tmp->buf,ptr,len);
    	tmp->size = len;
	rawpacket_t p = (rawpacket_t)CALLOC(g_rawpk_allocator,1,sizeof(*p));//calloc(1,sizeof(*p));    
    	packet_buf(p) = tmp;
	packet_type(p) = RAWPACKET;    
	packet_datasize(p) = len;
	packet_clone(p) = rawpacket_clone;
	packet_makeforwrite(p) = rawpacket_clone;
	packet_makeforread(p) = rawpacket_clone;	
    	return p; 	
}

static packet_t rawpacket_clone(packet_t other){
	if(!other || other->type != RAWPACKET) return NULL;
	rawpacket_t p = (rawpacket_t)CALLOC(g_rawpk_allocator,1,sizeof(*p));//calloc(1,sizeof(*p));
	packet_buf(p) = buffer_acquire(NULL,packet_buf(other));
	packet_begpos(p) = packet_begpos(other);
	packet_type(p) = RAWPACKET;	
	packet_datasize(p) = packet_datasize(other);
	packet_clone(p) = rawpacket_clone;
	packet_makeforwrite(p) = rawpacket_clone;
	packet_makeforread(p) = rawpacket_clone;	
	return (packet_t)p;
}

