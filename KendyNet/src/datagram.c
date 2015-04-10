#include "datagram.h"
#include <assert.h>

static inline void update_next_recv_pos(datagram_t c,int32_t _bytestransfer)
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

datagram_t new_datagram(handle_t sock,uint32_t buffersize,decoder *_decoder)
{
	buffersize = size_of_pow2(buffersize);
    	if(buffersize < 1024) buffersize = 1024;	
	datagram_t c = calloc(1,sizeof(*c));
	c->recv_bufsize = buffersize;
	c->next_recv_buf = buffer_create(buffersize);
	c->wrecvbuf[0].iov_len = buffersize;
	c->wrecvbuf[0].iov_base = c->next_recv_buf->buf;
	c->recv_overlap.iovec_count = 1;
	c->recv_overlap.iovec = c->wrecvbuf;	
	refobj_init((refobj*)c,connection_destroy);
	c->handle = sock;
	kn_sock_setud(sock,c);
	c->_decoder = _decoder;
	if(!c->_decoder) c->_decoder = new_rawpk_decoder();
	return c;
}


static inline void prepare_recv(connection_t c){
	buffer_t buf;
	uint32_t pos;
	int32_t i = 0;
	uint32_t free_buffer_size;
	uint32_t recv_size;
	//发出新的读请求
	buf = c->next_recv_buf;
	pos = c->next_recv_pos;
	recv_size = c->recv_bufsize;
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
}



static inline void PostRecv(datagram_t c){
	prepare_recv(c);
	kn_sock_post_recv(c->handle,&c->recv_overlap);	
	c->doing_recv = 1;	
}

static inline int Recv(datagram_t c,int32_t* err_code){
	prepare_recv(c);
	int ret = kn_sock_recv(c->handle,&c->recv_overlap);
	if(err_code) *err_code = errno;
	if(ret == 0){
		c->doing_recv = 1;
		return 0;
	}
	return ret;
}

void RecvFinish(datagram_t c,int32_t bytestransfer,int32_t err_code)
{
	c->doing_recv = 0;
	if(bytestransfer > 0){
		update_next_recv_pos(c,bytestransfer);
		c->_decoder->unpack(c->_decoder,c))
	}	
}

void IoFinish(handle_t sock,void *_,int32_t bytestransfer,int32_t err_code)
{
	st_io *io = ((st_io*)_);
	datagram_t c = kn_sock_getud(sock);
	refobj_inc((refobj*)c);
	if(io == (st_io*)&c->recv_overlap)
		RecvFinish(c,bytestransfer,err_code);
	refobj_dec((refobj*)c);
}

int datagram_send(datagram_t c,packet_t w,kn_sockaddr *addr)
{	
	if(!addr){
		errno = EINVAL;
		return -1;
	}
	if(packet_type(w) != WPACKET && packet_type(w) != RAWPACKET){
		errno = EMSGSIZE;
		return -1;
	}
	st_io o;
	int32_t i = 0;
	uint32_t size = 0;
	uint32_t pos = packet_begpos(w);
	buffer_t b = packet_buf(w);
	uint32_t buffer_size = packet_datasize(w);
	while(i < MAX_WBAF && b && buffer_size)
	{
		c->wsendbuf[i].iov_base = b->buf + pos;
		size = b->size - pos;
		size = size > buffer_size ? buffer_size:size;
		buffer_size -= size;
		c->wsendbuf[i].iov_len = size;
		++i;
		b = b->next;
		pos = 0;
	}
	if(buffer_size != 0){
		errno = EMSGSIZE;
		return -1;		
	}
	o.iovec_count = i;
	o.iovec = c->wsendbuf;
	o.addr = &addr;
	return kn_sock_send(c->handle,&o);
}

int datagram_associate(engine_t e,datagram_t conn,DCCB_PROCESS_PKT on_packet);
{		
      kn_engine_associate(e,conn->handle,IoFinish);
      if(on_packet) conn->on_packet = on_packet;
      if(e && !conn->doing_recv) PostRecv(conn);
      return 0;
}
