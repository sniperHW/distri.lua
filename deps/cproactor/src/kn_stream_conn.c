#include <assert.h>
#include "kn_stream_conn_private.h"
#include "kn_list.h"
#include "kn_time.h"
#include "kendynet.h"

static inline void update_next_recv_pos(kn_stream_conn_t c,int32_t _bytestransfer)
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
				c->next_recv_buf->next = buffer_create(c->recv_bufsize);
			c->next_recv_buf = buffer_acquire(c->next_recv_buf,c->next_recv_buf->next);
			c->next_recv_pos = 0;
		}
	}while(bytestransfer);
}

static inline int unpack(kn_stream_conn_t c)
{
	uint32_t pk_len = 0;
	uint32_t pk_total_size;
	rpacket_t r = NULL;
	if(c->is_close) return 0;//关闭之后的所有packet都不再提交
	do{
		if(!c->raw)
		{
			if(c->unpack_size <= sizeof(uint32_t))
				return 1;
			buffer_read(c->unpack_buf,c->unpack_pos,(int8_t*)&pk_len,sizeof(pk_len));
			pk_total_size = pk_len+sizeof(pk_len);
			if(pk_total_size > c->recv_bufsize){
				//可能是攻击
				return -1;
			}
			if(pk_total_size > c->unpack_size)
				return 1;
			r = rpk_create(c->unpack_buf,c->unpack_pos,pk_len,c->raw);
			r->base.tstamp = kn_systemms();
			do{
				uint32_t size = c->unpack_buf->size - c->unpack_pos;
				size = pk_total_size > size ? size:pk_total_size;
				c->unpack_pos  += size;
				pk_total_size  -= size;
				c->unpack_size -= size;
				if(c->unpack_pos >= c->unpack_buf->capacity)
				{
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
				assert(c->unpack_buf->next);
				c->unpack_pos = 0;
				c->unpack_buf = buffer_acquire(c->unpack_buf,c->unpack_buf->next);
			}
		}
		if(c->on_packet(c,r)) rpk_destroy(r);
		if(c->is_close) return 0;
	}while(1);

	return 1;
}


static inline st_io *prepare_send(kn_stream_conn_t c)
{
	int32_t i = 0;
    wpacket_t w = (wpacket_t)kn_list_head(&c->send_list);
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
		c->send_overlap.iovec_count = i;
		c->send_overlap.iovec = c->wsendbuf;
		O = (st_io*)&c->send_overlap;
	}
	return O;

}
static inline void update_send_list(kn_stream_conn_t c,int32_t _bytestransfer)
{
    assert(_bytestransfer >= 0);
	wpacket_t w;
    uint32_t bytestransfer = (uint32_t)_bytestransfer;
	uint32_t size;
	while(bytestransfer)
	{
        w = (wpacket_t)kn_list_pop(&c->send_list);
		assert(w);
		if((uint32_t)bytestransfer >= w->data_size)
		{
			bytestransfer -= w->data_size;
			wpk_destroy(w);
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
			kn_list_pushfront(&c->send_list,(kn_list_node*)w);
		}
	}
}

static void stream_conn_destroy(void *ptr)
{
	kn_fd_t fd = (kn_fd_t)((char*)ptr - sizeof(kn_dlist_node));
	kn_stream_conn_t c = (kn_stream_conn_t)fd->ud;
	if(c->_timer_item)
		kn_unregister_timer(&c->_timer_item);
    wpacket_t w;
    while((w = (wpacket_t)kn_list_pop(&c->send_list))!=NULL)
        wpk_destroy(w);
    buffer_release(&c->unpack_buf);
    buffer_release(&c->next_recv_buf);		
	c->fd_destroy_fn(ptr);
	if(c->service){
		kn_dlist_remove((kn_dlist_node*)c);
	}
	free(c);
}

kn_stream_conn_t kn_new_stream_conn(kn_fd_t s)
{
	kn_stream_conn_t c = calloc(1,sizeof(*c));
	//由s的释放来销毁kn_stream_conn_t
	c->fd_destroy_fn = s->ref.destroyer;
	s->ref.destroyer = stream_conn_destroy;
	c->fd = s;
	kn_fd_setud(s,(void*)c);
	return c;
}

void kn_stream_conn_close(kn_stream_conn_t c){
	if(c->is_close) return;
	c->is_close = 1;
	if(!c->doing_send){
		if(c->on_disconnected) c->on_disconnected(c,0);
		kn_closefd(c->fd);
	}else{
		//确保待发送数据发送完毕或发送超时才调用kn_closefd
		c->send_timeout = 30*1000;			
	} 
}

int32_t kn_stream_conn_send(kn_stream_conn_t c,wpacket_t w)
{
	if(c->is_close || (w->len && *w->len > c->recv_bufsize)){
		//包长度超大，记录日志
		wpk_destroy(w);
		return 0;
	}
	st_io *O;
	if(w){
		w->base.tstamp = kn_systemms64();
        kn_list_pushback(&c->send_list,(kn_list_node*)w);
	}
	if(!c->doing_send){
	    c->doing_send = 1;
		O = prepare_send(c);
		if(O) return kn_post_send(c->fd,O);
	}
	return 0;
}

void start_recv(kn_stream_conn_t c)
{
	c->unpack_buf = buffer_create(c->recv_bufsize);
	c->next_recv_buf = buffer_acquire(NULL,c->unpack_buf);
	c->wrecvbuf[0].iov_len = c->recv_bufsize;
	c->wrecvbuf[0].iov_base = c->next_recv_buf->buf;
	c->recv_overlap.iovec_count = 1;
	c->recv_overlap.iovec = c->wrecvbuf;
	c->last_recv = kn_systemms64();
	kn_post_recv(c->fd,&c->recv_overlap);
}

void RecvFinish(kn_stream_conn_t c,int32_t bytestransfer,int32_t err_code)
{
	uint32_t recv_size;
	uint32_t free_buffer_size;
	buffer_t buf;
	uint32_t pos;
	int32_t i = 0;
	do{
		if(bytestransfer == 0 || (bytestransfer < 0 && err_code != EAGAIN)){
			//不处理半关闭的情况，如果读到流的结尾直接关闭连接
			if(!c->is_close && c->on_disconnected)
				c->on_disconnected(c,err_code);
			kn_closefd(c->fd);	
			return;
		}else if(bytestransfer > 0){
			int32_t total_size = 0;
			do{
				c->last_recv = kn_systemms64();
				update_next_recv_pos(c,bytestransfer);
				c->unpack_size += bytestransfer;
				total_size += bytestransfer;
				int r = unpack(c);
				if(r < 0){
					//数据包错误，触发连接断开
					bytestransfer = -1;
					err_code = 0;
					break;
				}else if(r == 0)
					return;
				buf = c->next_recv_buf;
				pos = c->next_recv_pos;
				recv_size = c->recv_bufsize;
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
							buf->next = buffer_create(c->recv_bufsize);
						buf = buf->next;
					}
					++i;
				}while(recv_size);
				c->recv_overlap.iovec_count = i;
				c->recv_overlap.iovec = c->wrecvbuf;
				if(total_size >= c->recv_bufsize){
					kn_post_recv(c->fd,&c->recv_overlap);
					return;
				}
				else
					bytestransfer = kn_recv(c->fd,&c->recv_overlap,&err_code);
			}while(bytestransfer > 0);
		}else
			break;
	}while(1);
}

void SendFinish(kn_stream_conn_t c,int32_t bytestransfer,int32_t err_code)
{
	do{
		if(bytestransfer == 0 || (bytestransfer < 0 && err_code != EAGAIN)){
			//不处理半关闭的情况，如果对端关闭读直接关闭连接
			if(!c->is_close && c->on_disconnected)
				c->on_disconnected(c,err_code);
			kn_closefd(c->fd);	
			return;
		}else if(bytestransfer > 0){
			do{
                update_send_list(c,bytestransfer);
				st_io *io = prepare_send(c);
				if(!io) {
				    c->doing_send = 0;
				    if(c->is_close){
						//数据发送完毕且收到关闭请求，可以安全关闭了
						c->on_disconnected(c,0); 
						kn_closefd(c->fd);
					}
					return;
				}
				bytestransfer = kn_send(c->fd,io,&err_code);
			}while(bytestransfer > 0);
		}else 
			break;
	}while(1);
}

void IoFinish(kn_fd_t fd,st_io *io,int32_t bytestransfer,int32_t err_code)
{
	kn_stream_conn_t c = kn_fd_getud(fd);
    if(io == (st_io*)&c->send_overlap)
        SendFinish(c,bytestransfer,err_code);
    else if(io == (st_io*)&c->recv_overlap)
        RecvFinish(c,bytestransfer,err_code);
    else{
		if(c->on_disconnected)
			c->on_disconnected(c,err_code);		
		kn_closefd(c->fd);
	}
}

void  kn_stream_conn_setud(kn_stream_conn_t conn,void *ud)
{
	conn->ud = ud;
}

void *kn_stream_conn_getud(kn_stream_conn_t conn)
{
	return conn->ud;
}
