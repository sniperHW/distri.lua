#ifndef _RAWPACKET_H
#define _RAWPACKET_H

#include "packet.h"
#include "kn_common_define.h"
#include "kn_alloc.h"


extern allocator_t g_rawpk_allocator;

typedef struct rawpacket
{
	struct packet base;
}*rawpacket_t;

rawpacket_t rawpacket_create1(buffer_t b,uint32_t pos,uint32_t len);

rawpacket_t rawpacket_create2(void *ptr,uint32_t len);

//rawpacket_t rawpacket_copy_create(packet_t other);

static inline void* rawpacket_data(rawpacket_t p,uint32_t *len){
	if(!packet_buf(p)) return NULL;
	if(len) *len = packet_datasize(p);
	return &(packet_buf(p)->buf[packet_begpos(p)]);
}


#endif
