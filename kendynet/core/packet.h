#ifndef _PACKET_H
#define _PACKET_H

#include "buffer.h"
#include "link_list.h"
#include <stdint.h>
#include "allocator.h"
#include <assert.h>

//base of wpacket and rpacket
struct packet
{
	list_node next;
	uint8_t   type;        //packet type see common_define.h
	buffer_t  buf;
	uint8_t   raw;
	union{
        uint64_t  usr_data;
        void     *usr_ptr;
	};
	uint32_t  begin_pos;
	uint32_t  tstamp;
};

#endif
