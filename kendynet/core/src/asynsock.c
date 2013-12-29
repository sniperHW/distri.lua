#include "asynsock.h"

void asynsock_destroy(void *ptr)
{
    asynsock_t sock = (asynsock_t)ptr;
	if(sock->c) 
		release_conn(sock->c);
	else if(sock->s != INVALID_SOCK) 
		CloseSocket(sock->s);
	free(sock);
}

asynsock_t asynsock_new(struct connection *c,SOCK s)
{
    asynsock_t sock = calloc(1,sizeof(*sock));
    ref_init(&sock->ref,asynsock_destroy,1);
    if(c){
		sock->c = c;
        c->usr_ptr = sock;
    }
	else if(s != INVALID_SOCK)
		sock->s = s;
	else
	{
		free(sock);
		return NULL;
	}
    ident _ident = make_ident(&sock->ref);
    sock->sident = CAST_2_SOCK(_ident);
	return sock;
}
