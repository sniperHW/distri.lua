#include "kendynet.h"
#include "packet.h"
#include "wpacket.h"
#include "rpacket.h"
#include "rawpacket.h"

static inline void rawpacket_destroy(rawpacket_t p)
{
	//释放所有对buffer_t的引用
	buffer_release(packet_buf(p));
	free(p);
}

void rpk_destroy(rpacket_t r)
{
	//释放所有对buffer_t的引用
	buffer_release(packet_buf(r));
	buffer_release(r->readbuf);
	buffer_release(r->binbuf);
	free(r);
}

void wpk_destroy(wpacket_t w)
{
	buffer_release(packet_buf(w));
	buffer_release(w->writebuf);
	free(w);
}

void _destroy_packet(packet_t p){
	if(packet_type(p) == RPACKET) rpk_destroy((rpacket_t)p);
	else if(packet_type(p) == WPACKET) wpk_destroy((wpacket_t)p);
	else if(packet_type(p) == RAWPACKET) rawpacket_destroy((rawpacket_t)p);
}

packet_t packet_copy_create(packet_t p){
	if(p->type == RAWPACKET) 
		return (packet_t)rawpacket_copy_create(p);
	else if(p->type == WPACKET)
		return (packet_t)wpk_copy_create(p);
	else if(p->type == RPACKET)
		return (packet_t)rpk_copy_create(p);
	return NULL;
}