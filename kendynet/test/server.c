//a low level pingpong server

#include "core/kendynet.h"

struct session;

struct OVERLAPCONTEXT
{
	st_io   m_super;
	struct  session *s;
};


struct session{
	struct OVERLAPCONTEXT send_overlap;
	struct OVERLAPCONTEXT recv_overlap;
	struct iovec wbuf[1];
	char   buf[65536];
	SOCK   sock;
};


void session_recv(struct session *s)
{
	s->wbuf[0].iov_base = s->buf;
	s->wbuf[0].iov_len = 65536;
	s->recv_overlap.m_super.iovec_count = 1;
	s->recv_overlap.m_super.iovec = s->wbuf;
	s->recv_overlap.s = s;
	Post_Recv(s->sock,(st_io*)&s->recv_overlap);
}

void session_send(struct session *s,int32_t size)
{
	s->wbuf[0].iov_base = s->buf;
   	s->wbuf[0].iov_len = size;
	s->send_overlap.m_super.iovec_count = 1;
	s->send_overlap.m_super.iovec = s->wbuf;
	s->send_overlap.s = s;     	
    Post_Send(s->sock,(st_io*)&s->send_overlap); 
}

void IoFinish(int32_t bytestransfer,st_io *io,uint32_t err_code)
{
	
    struct OVERLAPCONTEXT *OVERLAP = (struct OVERLAPCONTEXT *)io;
    struct session *s = OVERLAP->s;
    
	if(bytestransfer < 0 && err_code != EAGAIN){
		printf("socket close\n");
        CloseSocket(s->sock);
        free(s);
        return;
	}
    if(io == (st_io*)&s->send_overlap)
		session_recv(s);
    else if(io == (st_io*)&s->recv_overlap)
		session_send(s,bytestransfer);
}

void cb_accept(SOCK sock,struct sockaddr_in *addr,void *ud)
{
	ENGINE e = (ENGINE)ud;
	struct session *s = calloc(1,sizeof(*s));
	s->sock = sock;    	
	Bind2Engine(e,s->sock,IoFinish);
	session_recv(s);   
}


int main(int argc,char **argv)
{
	ENGINE  e = CreateEngine();
	EListen(e,argv[1],atoi(argv[2]),(void*)e,cb_accept);
	while(1)
		EngineRun(e,50);
	return 0;
}
