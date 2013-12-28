#include "datasocket.h"

void destroy_datasocket(void *ptr)
{
	datasocket_t sock = (datasocket_t)ptr;
	if(sock->c) 
		release_conn(sock->c);
	else if(sock->s != INVALID_SOCK) 
		CloseSocket(sock->s);
	free(sock);
}

datasocket_t create_datasocket(struct connection *c,SOCK s)
{
	datasocket_t sock = calloc(1,sizeof(*sock));
	ref_init(&sock->ref,destroy_datasocket,1);
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
