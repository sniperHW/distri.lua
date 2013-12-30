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
#include "llist.h"
#include <stdint.h>
#include "allocator.h"
#include <assert.h>

//base of wpacket and rpacket
struct packet
{
    lnode     next;
	uint8_t   type;        //packet type see common_define.h
	buffer_t  buf;
	uint8_t   raw;
	union{
        uint64_t  usr_data;
        void     *usr_ptr;
		ident     _ident;
	};
	uint32_t  begin_pos;
	uint32_t  tstamp;
};

#endif
