struct session{
	st_io send_overlap;
	st_io recv_overlap;
	struct iovec wbuf[1];
	char   buf[65535];
	handle_t   s;
};

void session_recv(struct session *s)
{
	s->wbuf[0].iov_base = s->buf;
	s->wbuf[0].iov_len = 65535;
	s->recv_overlap.iovec_count = 1;
	s->recv_overlap.iovec = s->wbuf;
	kn_sock_post_recv(s->s,&s->recv_overlap);
}

void session_send(struct session *s,int32_t size)
{
	s->wbuf[0].iov_base = s->buf;
   	s->wbuf[0].iov_len = size;
	s->send_overlap.iovec_count = 1;
	s->send_overlap.iovec = s->wbuf;  	
    	kn_sock_post_send(s->s,&s->send_overlap); 
}

int      client_count = 0;
uint64_t totalbytes   = 0; 

void transfer_finish(handle_t s,void *_,int32_t bytestransfer,int32_t err){
    st_io *io = ((st_io*)_); 
    struct session *session = kn_sock_getud(s);
    if(!io || bytestransfer <= 0)
    {
        printf("disconnected,%d\n",kn_get_ssl_error(s,0));
        kn_close_sock(s);
        free(session);
        --client_count;           
        return;
    }	
    if(io == &session->send_overlap)
		session_recv(session);
    else if(io == &session->recv_overlap){
		session_send(session,bytestransfer);
		totalbytes += bytestransfer;
	}
}
