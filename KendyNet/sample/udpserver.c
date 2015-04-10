#include "kendynet.h"
#include "kn_timer.h"

struct datagram{
	st_io send_overlap;
	st_io recv_overlap;
	struct iovec wbuf[1];
	char   buf[65535];
	handle_t   s;
};

void datagram_recv(struct datagram *s)
{
	s->wbuf[0].iov_base = s->buf;
	s->wbuf[0].iov_len = 65535;
	s->recv_overlap.iovec_count = 1;
	s->recv_overlap.iovec = s->wbuf;
	kn_sock_post_recv(s->s,&s->recv_overlap);
}

void datagram_send(struct datagram *s,int32_t size)
{
	s->send_overlap.addr = s->recv_overlap.addr;
	s->wbuf[0].iov_base = s->buf;
   	s->wbuf[0].iov_len = size;
	s->send_overlap.iovec_count = 1;
	s->send_overlap.iovec = s->wbuf;  	
    	kn_sock_send(s->s,&s->send_overlap);
    	datagram_recv(s); 
}

double totalbytes   = 0; 

void transfer_finish(handle_t s,void *_,int32_t bytestransfer,int32_t err){
    	st_io *io = ((st_io*)_); 
    	struct datagram *session = kn_sock_getud(s);
	if(!io || bytestransfer <= 0)
	{
		kn_close_sock(s);
		exit(0);      
		return;
	}	
	if(io == &session->recv_overlap){
		datagram_send(session,bytestransfer);
		totalbytes += bytestransfer;
	}
}

int timer_callback(kn_timer_t timer){
	printf("totalbytes:%f MB/s\n",totalbytes/1024/1024);
	totalbytes = 0;
	return 1;
}

int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);
	engine_t p = kn_new_engine();
	kn_sockaddr local;
	kn_addr_init_in(&local,argv[1],atoi(argv[2]));
	handle_t l = kn_new_sock(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	kn_sock_listen(l,&local);
	kn_engine_associate(p,l,transfer_finish);
	struct datagram d = {.s = l};
	kn_sock_setud(l,&d);
	datagram_recv(&d);
	kn_reg_timer(p,1000,timer_callback,NULL);		
	kn_engine_run(p);
	return 0;
}