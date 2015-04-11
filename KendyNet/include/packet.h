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
#ifndef _PACKET_H
#define _PACKET_H

#include "buffer.h"
#include "kn_list.h"
#include <stdint.h>
#include <assert.h>

enum{
	WPACKET = 1,
	RPACKET,
	RAWPACKET,
	PACKET_END,
};

typedef struct packet
{
    kn_list_node  listnode;
    uint8_t        type;    
    buffer_t      buf;    
    uint32_t      begin_pos;
    union{    
        uint32_t      data_size;      //for write and raw
        uint32_t      data_remain;//for read
    };
    struct packet * (*clone)(struct packet*);
    struct packet*  (*makeforwrite)(struct packet*);
    struct packet*  (*makeforread)(struct packet*);
}packet,*packet_t;

void _destroy_packet(packet_t);

static inline packet_t clone_packet(packet_t p){
    return p->clone(p);
}

static inline packet_t make_readpacket(packet_t p){
    return p->makeforread(p);
}

static inline packet_t make_writepacket(packet_t p){
    return p->makeforwrite(p);
} 


#define destroy_packet(p) _destroy_packet((packet_t)(p))
#define packet_next(p)   ((packet_t)(p))->listnode.next
#define packet_buf(p)    ((packet_t)(p))->buf
#define packet_type(p)   ((packet_t)(p))->type
#define packet_begpos(p) ((packet_t)(p))->begin_pos
#define packet_datasize(p) ((packet_t)(p))->data_size
#define packet_dataremain(p) ((packet_t)(p))->data_remain
#define packet_clone(p) ((packet_t)(p))->clone
#define packet_makeforwrite(p) ((packet_t)(p))->makeforwrite
#define packet_makeforread(p) ((packet_t)(p))->makeforread

#endif
