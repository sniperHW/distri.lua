
struct session{
	st_io send_overlap;
	st_io recv_overlap;
	struct iovec wbuf[1];
	char   buf[65536];
	kn_fd_t   s;
};

void session_recv(struct session *s)
{
	s->wbuf[0].iov_base = s->buf;
	s->wbuf[0].iov_len = 65536;
	s->recv_overlap.iovec_count = 1;
	s->recv_overlap.iovec = s->wbuf;
	kn_post_recv(s->s,&s->recv_overlap);
}

void session_send(struct session *s,int32_t size)
{
	s->wbuf[0].iov_base = s->buf;
   	s->wbuf[0].iov_len = size;
	s->send_overlap.iovec_count = 1;
	s->send_overlap.iovec = s->wbuf;  	
    kn_post_send(s->s,&s->send_overlap); 
}

int      client_count = 0;
uint64_t totalbytes   = 0; 

void transfer_finish(kn_fd_t s,st_io *io,int32_t bytestransfer,int32_t err){
    struct session *session = kn_fd_getud(s);
    if(!io || bytestransfer <= 0)
    {
        kn_closefd(s);
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

