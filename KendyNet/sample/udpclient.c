#include "kendynet.h"

struct datagram{
	st_io send_overlap;
	st_io recv_overlap;
	struct iovec wbuf[1];
	char   buf[65535];
	handle_t   s;
};

const char *ip;
int      port;

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
	kn_addr_init_in(&s->send_overlap.addr,ip,port);	
	s->wbuf[0].iov_base = s->buf;
   	s->wbuf[0].iov_len = size;
	s->send_overlap.iovec_count = 1;
	s->send_overlap.iovec = s->wbuf;  	
    	kn_sock_post_send(s->s,&s->send_overlap); 
}


uint64_t totalbytes   = 0; 

void transfer_finish(handle_t s,void *_,int32_t bytestransfer,int32_t err){
    st_io *io = ((st_io*)_); 
    struct datagram *session = kn_sock_getud(s);
    if(!io || bytestransfer <= 0)
    {
        kn_close_sock(s);
        exit(0);         
        return;
    }	
    if(io == &session->send_overlap){
		datagram_recv(session);
    }
    else if(io == &session->recv_overlap){
		datagram_send(session,bytestransfer);
		totalbytes += bytestransfer;
    }
}

int main(int argc,char **argv){
	ip = argv[1];
	port = atoi(argv[2]);
	signal(SIGPIPE,SIG_IGN);
	engine_t p = kn_new_engine();
	handle_t l = kn_new_sock(AF_INET,SOCK_DGRAM,IPPROTO_UDP);	
	kn_engine_associate(p,l,transfer_finish);
	struct datagram d = {.s = l};
	kn_sock_setud(l,&d);
	datagram_send(&d,200);	
	kn_engine_run(p);
	return 0;
}