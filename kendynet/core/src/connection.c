#include "connection.h"
#include "llist.h"
#include <assert.h>
#include "systime.h"
#include "socket.h"

#define BUFFER_SIZE 65536

//接收相关函数
static inline void update_next_recv_pos(struct connection *c,int32_t _bytestransfer)
{
    assert(_bytestransfer >= 0);
    uint32_t bytestransfer = (uint32_t)_bytestransfer;
    uint32_t size;
	do{
		size = c->next_recv_buf->capacity - c->next_recv_pos;
        size = size > bytestransfer ? bytestransfer:size;
		c->next_recv_buf->size += size;
		c->next_recv_pos += size;
		bytestransfer -= size;
		if(c->next_recv_pos >= c->next_recv_buf->capacity)
		{
			if(!c->next_recv_buf->next)
				c->next_recv_buf->next = buffer_create_and_acquire(NULL,BUFFER_SIZE);
			c->next_recv_buf = buffer_acquire(c->next_recv_buf,c->next_recv_buf->next);
			c->next_recv_pos = 0;
		}
	}while(bytestransfer);
}

static inline int unpack(struct connection *c)
{
	uint32_t pk_len = 0;
	uint32_t pk_total_size;
	rpacket_t r = NULL;
	do{
		if(!c->raw)
		{
			if(c->unpack_size <= sizeof(uint32_t))
				return 1;
			buffer_read(c->unpack_buf,c->unpack_pos,(int8_t*)&pk_len,sizeof(pk_len));
			pk_total_size = pk_len+sizeof(pk_len);
			if(pk_total_size > c->unpack_size)
				return 1;
			r = rpk_create(c->unpack_buf,c->unpack_pos,pk_len,c->raw);
			r->base.tstamp = GetSystemSec();
			//调整unpack_buf和unpack_pos
			do{
				uint32_t size = c->unpack_buf->size - c->unpack_pos;
				size = pk_total_size > size ? size:pk_total_size;
				c->unpack_pos  += size;
				pk_total_size  -= size;
				c->unpack_size -= size;
				if(c->unpack_pos >= c->unpack_buf->capacity)
				{
					/* unpack之前先执行了update_next_recv_pos,在update_next_recv_pos中
					*  如果buffer被填满，会扩展一块新的buffer加入链表中，所以这里的
					*  c->unpack_buf->next不应该会是NULL
					*/
					assert(c->unpack_buf->next);
					c->unpack_pos = 0;
					c->unpack_buf = buffer_acquire(c->unpack_buf,c->unpack_buf->next);
				}
			}while(pk_total_size);
		}
		else
		{
			pk_len = c->unpack_buf->size - c->unpack_pos;
            if(!pk_len) return 1;
			r = rpk_create(c->unpack_buf,c->unpack_pos,pk_len,c->raw);
			c->unpack_pos  += pk_len;
			c->unpack_size -= pk_len;
			if(c->unpack_pos >= c->unpack_buf->capacity)
			{
				/* unpack之前先执行了update_next_recv_pos,在update_next_recv_pos中
				*  如果buffer被填满，会扩展一块新的buffer加入链表中，所以这里的
				*  c->unpack_buf->next不应该会是NULL
				*/
				assert(c->unpack_buf->next);
				c->unpack_pos = 0;
				c->unpack_buf = buffer_acquire(c->unpack_buf,c->unpack_buf->next);
			}
		}
        if(r){
            int8_t process_ret = 1;
            if(test_recvable(c->status))
                process_ret = c->cb_process_packet(c,r);
            if(process_ret) rpk_destroy(&r);
        }
        if(!test_recvable(c->status)) return 0;
	}while(1);

	return 1;
}

//发送相关函数
static inline st_io *prepare_send(struct connection *c)
{
	int32_t i = 0;
    wpacket_t w = (wpacket_t)llist_head(&c->send_list);
	buffer_t b;
	uint32_t pos;
	st_io *O = NULL;
	uint32_t buffer_size = 0;
	uint32_t size = 0;
	uint32_t send_size_remain = MAX_SEND_SIZE;
	while(w && i < MAX_WBAF && send_size_remain > 0)
	{
		pos = w->base.begin_pos;
		b = w->base.buf;
		buffer_size = w->data_size;
		while(i < MAX_WBAF && b && buffer_size && send_size_remain > 0)
		{
			c->wsendbuf[i].iov_base = b->buf + pos;
			size = b->size - pos;
			size = size > buffer_size ? buffer_size:size;
			size = size > send_size_remain ? send_size_remain:size;
			buffer_size -= size;
			send_size_remain -= size;
			c->wsendbuf[i].iov_len = size;
			++i;
			b = b->next;
			pos = 0;
		}
        if(send_size_remain > 0) w = (wpacket_t)MSG_NEXT(w);//(wpacket_t)w->base.next.next;
	}
	if(i){
		c->send_overlap.m_super.iovec_count = i;
		c->send_overlap.m_super.iovec = c->wsendbuf;
		O = (st_io*)&c->send_overlap;
	}
	return O;

}
static inline void update_send_list(struct connection *c,int32_t _bytestransfer)
{
    assert(_bytestransfer >= 0);
	wpacket_t w;
    uint32_t bytestransfer = (uint32_t)_bytestransfer;
	uint32_t size;
	while(bytestransfer)
	{
        w = LLIST_POP(wpacket_t,&c->send_list);
		assert(w);
		if((uint32_t)bytestransfer >= w->data_size)
		{
			bytestransfer -= w->data_size;
			wpk_destroy(&w);
		}
		else
		{
			while(bytestransfer)
			{
				size = w->base.buf->size - w->base.begin_pos;
				size = size > (uint32_t)bytestransfer ? (uint32_t)bytestransfer:size;
				bytestransfer -= size;
				w->base.begin_pos += size;
				w->data_size -= size;
				if(w->base.begin_pos >= w->base.buf->size)
				{
					w->base.begin_pos = 0;
					w->base.buf = buffer_acquire(w->base.buf,w->base.buf->next);
				}
			}
            LLIST_PUSH_FRONT(&c->send_list,w);
		}
	}
}

int32_t send_packet(struct connection *c,wpacket_t w)
{
    if(!test_sendable(c->status)){
		wpk_destroy(&w);
		return -1;
	}
	st_io *O;
	if(w){
		w->base.tstamp = GetSystemMs64();
        LLIST_PUSH_BACK(&c->send_list,w);
	}
	if(!c->doing_send){
	    c->doing_send = 1;
		O = prepare_send(c);
		if(O) return Post_Send(c->socket,O);
	}
	return 0;
}

static inline void start_recv(struct connection *c)
{
    if(test_recvable(c->status)){
        c->unpack_buf = buffer_create_and_acquire(NULL,BUFFER_SIZE);
        c->next_recv_buf = buffer_acquire(NULL,c->unpack_buf);
        c->wrecvbuf[0].iov_len = BUFFER_SIZE;
        c->wrecvbuf[0].iov_base = c->next_recv_buf->buf;
        c->recv_overlap.m_super.iovec_count = 1;
        c->recv_overlap.m_super.iovec = c->wrecvbuf;
        c->last_recv = GetSystemMs64();
        Post_Recv(c->socket,&c->recv_overlap.m_super);
    }
}

void active_close(struct connection *c)
{
    if(c->status & SESTABLISH){
        if(LLIST_IS_EMPTY(&c->send_list)){
            //没有数据需要发送了直接关闭
            c->status = SCLOSE;
			CloseSocket(c->socket);
            printf("active close\n");
            c->cb_disconnect(c,0);
		}else
        {
            //还有数据需要发送，等数据发送完毕后再关闭
            ShutDownRecv(c->socket);
            c->status = (c->status | SWAITCLOSE | SRCLOSE);
        }
    }
}

struct socket_wrapper *get_socket_wrapper(SOCK s);
void RecvFinish(int32_t bytestransfer,struct connection *c,uint32_t err_code)
{
	uint32_t recv_size;
	uint32_t free_buffer_size;
	buffer_t buf;
	uint32_t pos;
	int32_t i = 0;
	do{
		if(bytestransfer == 0)
			return;
		else if(bytestransfer < 0 && err_code != EAGAIN){
			//printf("recv close\n");
            if(c->status != SCLOSE){
                c->status = SCLOSE;
                CloseSocket(c->socket);
                //被动关闭
                c->cb_disconnect(c,err_code);
			}
			return;
		}else if(bytestransfer > 0){
			int32_t total_size = 0;
			do{
				c->last_recv = GetSystemMs64();
				update_next_recv_pos(c,bytestransfer);
				c->unpack_size += bytestransfer;
				total_size += bytestransfer;
				if(!unpack(c)) return;
				buf = c->next_recv_buf;
				pos = c->next_recv_pos;
				recv_size = BUFFER_SIZE;
				i = 0;
				do
				{
					free_buffer_size = buf->capacity - pos;
					free_buffer_size = recv_size > free_buffer_size ? free_buffer_size:recv_size;
					c->wrecvbuf[i].iov_len = free_buffer_size;
					c->wrecvbuf[i].iov_base = buf->buf + pos;
					recv_size -= free_buffer_size;
					pos += free_buffer_size;
					if(recv_size && pos >= buf->capacity)
					{
						pos = 0;
						if(!buf->next)
							buf->next = buffer_create_and_acquire(NULL,BUFFER_SIZE);
						buf = buf->next;
					}
					++i;
				}while(recv_size);
				c->recv_overlap.m_super.iovec_count = i;
				c->recv_overlap.m_super.iovec = c->wrecvbuf;
				if(total_size >= BUFFER_SIZE)
				{
					Post_Recv(c->socket,&c->recv_overlap.m_super);
					return;
				}
				else
					bytestransfer = Recv(c->socket,&c->recv_overlap.m_super,&err_code);
			}while(bytestransfer > 0);
		}
	}while(1);
}

void SendFinish(int32_t bytestransfer,struct connection *c,uint32_t err_code)
{
	do{
		if(bytestransfer == 0)
		    return;
		else if(bytestransfer < 0 && err_code != EAGAIN){
			//printf("send close\n");
            if(c->status & SESTABLISH)
            {
				ShutDownSend(c->socket);
                c->status = (c->status | SWCLOSE);
            }else if(c->status & SWAITCLOSE)
            {
                c->status = SCLOSE;
                CloseSocket(c->socket);
                c->cb_disconnect(c,0);
            }
			return;
		}else if(bytestransfer > 0)
		{
			do{
                update_send_list(c,bytestransfer);
				st_io *io = prepare_send(c);
				if(!io) {
				    c->doing_send = 0;
                    if(c->status & SWAITCLOSE)
					{
                        //数据已经发送完毕，关闭
                        c->status = SCLOSE;
                        CloseSocket(c->socket);
                        c->cb_disconnect(c,0);
					}
				    return;
				}
				bytestransfer = Send(c->socket,io,&err_code);
			}while(bytestransfer > 0);
		}
	}while(1);
}

void IoFinish(int32_t bytestransfer,st_io *io,uint32_t err_code)
{
    struct OVERLAPCONTEXT *OVERLAP = (struct OVERLAPCONTEXT *)io;
    struct connection *c = OVERLAP->c;
	acquire_conn(c);
    if(io == (st_io*)&c->send_overlap)
        SendFinish(bytestransfer,c,err_code);
    else if(io == (st_io*)&c->recv_overlap)
        RecvFinish(bytestransfer,c,err_code);
	release_conn(c);
}


void connection_destroy(void *arg)
{
	struct connection *c = (struct connection*)arg;
	unregister_timer(con2wheelitem(c));
    wpacket_t w;
    while((w = LLIST_POP(wpacket_t,&c->send_list))!=NULL)
        wpk_destroy(&w);
    buffer_release(&c->unpack_buf);
    buffer_release(&c->next_recv_buf);
    free(c);
    //printf("connection_destroy\n");
}

struct connection *new_conn(SOCK s,uint8_t is_raw)
{
	struct connection *c = calloc(1,sizeof(*c));
	c->socket = s;
	c->recv_overlap.c = c;
	c->send_overlap.c = c;
	c->raw = is_raw;
	ref_init(&c->ref,0,connection_destroy,1);
    c->status = SESTABLISH;
	return c;
}

void release_conn(struct connection *con){
	ref_decrease((struct refbase *)con);
}

void acquire_conn(struct connection *con){
	ref_increase((struct refbase *)con);
}

int32_t bind2engine(ENGINE e,struct connection *c,
    CCB_PROCESS_PKT cb_process_packet,CCB_DISCONNECT cb_disconnect)
{
    c->cb_process_packet = cb_process_packet;
    c->cb_disconnect = cb_disconnect;
	start_recv(c);
	return Bind2Engine(e,c->socket,IoFinish);
}
