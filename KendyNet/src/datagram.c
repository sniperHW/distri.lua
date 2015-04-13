#include "datagram.h"
#include <assert.h>

static void datagram_destroy(void *ptr)
{
	datagram_t c = (datagram_t)ptr;
	buffer_release(c->recv_buf);
	kn_close_sock(c->handle);
	destroy_decoder(c->_decoder);
	if(c->ud && c->destroy_ud){
		c->destroy_ud(c->ud);
	}
	free(c);				
}

static inline datagram_t _new_datagram(handle_t sock,uint32_t buffersize,decoder *_decoder)
{
	buffersize = size_of_pow2(buffersize);
    	if(buffersize < 1024) buffersize = 1024;	
	datagram_t c = calloc(1,sizeof(*c));
	c->recv_bufsize = buffersize;
	refobj_init((refobj*)c,datagram_destroy);
	c->handle = sock;
	kn_sock_setud(sock,c);
	c->_decoder = _decoder;
	if(!c->_decoder) c->_decoder = new_datagram_rawpk_decoder();
	return c;
}

datagram_t new_datagram(int domain,uint32_t buffersize,decoder *_decoder){
	handle_t l = kn_new_sock(domain,SOCK_DGRAM,IPPROTO_UDP);
	if(!l) 
		return NULL;
	return 	_new_datagram(l,buffersize,_decoder);	
}

static inline void prepare_recv(datagram_t c){
	if(c->recv_buf)
		buffer_release(c->recv_buf);
	c->recv_buf = buffer_create(c->recv_bufsize);
	c->wrecvbuf.iov_len = c->recv_bufsize;
	c->wrecvbuf.iov_base = c->recv_buf->buf;
	c->recv_overlap.iovec_count = 1;
	c->recv_overlap.iovec = &c->wrecvbuf;	
}

static inline void PostRecv(datagram_t c){
	prepare_recv(c);
	kn_sock_post_recv(c->handle,&c->recv_overlap);	
	c->doing_recv = 1;	
}

static int raw_unpack(decoder *_,void* _1){
	((void)_);
	datagram_t c = (datagram_t)_1;
	packet_t r = (packet_t)rawpacket_create1(c->recv_buf,0,c->recv_buf->size);
	c->on_packet(c,r,&c->recv_overlap.addr); 
	destroy_packet(r);
	return 0;
}

static int rpk_unpack(decoder *_,void *_1){
	((void)_);
	datagram_t c = (datagram_t)_1;
	if(c->recv_buf->size <= sizeof(uint32_t))
		return 0;	
	uint32_t pk_len = 0;
	uint32_t pk_hlen;
	buffer_read(c->recv_buf,0,(int8_t*)&pk_len,sizeof(pk_len));
	pk_hlen = kn_ntoh32(pk_len);
	uint32_t pk_total_size = pk_hlen+sizeof(pk_len);
	if(c->recv_buf->size < pk_total_size)
		return 0;	
	packet_t r = (packet_t)rpk_create(c->recv_buf,0,pk_len);
	c->on_packet(c,r,&c->recv_overlap.addr); 
	destroy_packet(r);	
	return 0;
}	

static void IoFinish(handle_t sock,void *_,int32_t bytestransfer,int32_t err_code)
{
	datagram_t c = kn_sock_getud(sock);
	c->doing_recv = 0;	
	refobj_inc((refobj*)c);
	if(bytestransfer > 0 && ((st_io*)_)->recvflags != MSG_TRUNC){
		c->recv_buf->size = bytestransfer;
		c->_decoder->unpack(c->_decoder,c);
	}
	PostRecv(c);
	refobj_dec((refobj*)c);
}

int datagram_send(datagram_t c,packet_t w,kn_sockaddr *addr)
{
	int ret = -1;
	do{	
		if(!addr){
			errno = EINVAL;
			break;
		}
		if(packet_type(w) != WPACKET && packet_type(w) != RAWPACKET){
			errno = EMSGSIZE;
			break;
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
			break;		
		}
		o.iovec_count = i;
		o.iovec = c->wsendbuf;
		o.addr = *addr;
		ret = kn_sock_send(c->handle,&o);
	}while(0);
	destroy_packet(w);
	return ret;
}

int datagram_associate(engine_t e,datagram_t conn,DCCB_PROCESS_PKT on_packet)
{		
      kn_engine_associate(e,conn->handle,IoFinish);
      if(on_packet) conn->on_packet = on_packet;
      if(e && !conn->doing_recv) PostRecv(conn);
      return 0;
}

decoder* new_datagram_rpk_decoder(){
	datagram_rpk_decoder *de = calloc(1,sizeof(*de));
	de->base.unpack = rpk_unpack;
	de->base.destroy = NULL;
	return (decoder*)de;
}

decoder* new_datagram_rawpk_decoder(){
	datagram_rawpk_decoder *de = calloc(1,sizeof(*de));
	de->base.destroy = NULL;
	de->base.unpack = raw_unpack;
	return (decoder*)de;	
}

void datagram_close(datagram_t c){
	refobj_dec((refobj*)c); 	
}

datagram_t datagram_listen(kn_sockaddr *local,uint32_t buffersize,decoder *_decoder){
	do{
		if(!local) break;
		handle_t l = kn_new_sock(local->addrtype,SOCK_DGRAM,IPPROTO_UDP);
		if(!l) break;
		if(0 != kn_sock_listen(l,local)){
			kn_close_sock(l);
			break;
		}
		return _new_datagram(l,buffersize,_decoder);
	}while(0);
	return NULL;
}