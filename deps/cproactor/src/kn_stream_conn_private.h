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
#ifndef _KN_STREAM_CONN_PRIVATE_H
#define _KN_STREAM_CONN_PRIVATE_H
#include "wpacket.h"
#include "rpacket.h"
#include <stdint.h>
#include "kn_timer.h"
#include "kn_allocator.h"
#include "kn_common_define.h"
#include "kn_stream_conn.h"
#include "kn_fd.h"

/*
*   返回1：process_packet调用后rpacket_t自动销毁
*   否则,将由使用者自己销毁
*/
typedef int  (*CCB_PROCESS_PKT)(kn_stream_conn_t,rpacket_t);

typedef void (*CCB_RECV_TIMEOUT)(kn_stream_conn_t);
typedef void (*CCB_SEND_TIMEOUT)(kn_stream_conn_t);

#define MAX_WBAF 512
#define MAX_SEND_SIZE 65536

struct kn_stream_conn
{
	kn_fd  fd;
	struct iovec wsendbuf[MAX_WBAF];
	struct iovec wrecvbuf[2];
	
    st_io    send_overlap;
	st_io    recv_overlap;
    void*    ud;
	uint32_t unpack_size; //还未解包的数据大小
	uint32_t unpack_pos;
	uint32_t next_recv_pos;
	buffer_t next_recv_buf;
	buffer_t unpack_buf;

    kn_list  send_list;//待发送的包
    CCB_PROCESS_PKT cb_process_packet;
	uint64_t last_recv;
	struct timer_item *_timer_item;
	uint32_t recv_timeout;
    uint32_t send_timeout;
    CCB_RECV_TIMEOUT cb_recv_timeout;
    CCB_SEND_TIMEOUT cb_send_timeout;
	uint8_t  raw;
    volatile uint32_t status;
	uint8_t  doing_send;
	uint32_t recv_bufsize;
};


/*
struct connection *new_conn(SOCK s,uint8_t is_raw);
void   release_conn(struct connection *con);
void   acquire_conn(struct connection *con);

void   active_close(struct connection*);//active close connection

int32_t send_packet(struct connection*,wpacket_t);

int32_t bind2engine(ENGINE,struct connection*,uint32_t recv_bufsize,CCB_PROCESS_PKT,CCB_DISCONNECT);
*/

#endif
