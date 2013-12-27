#include "datasocket.h"

void destroy_datasocket(void *ptr)
{
	datasocket_t sock = (datasocket_t)ptr;
	release_conn(sock->c);
	free(sock);
}

datasocket_t create_datasocket(struct connection *c,SOCK s)
{
	datasocket_t sock = calloc(1,sizeof(*sock));
	ref_init(&sock->ref,destroy_datasocket,1);
	if(c)
		sock->c = c;
	else if(s != INVALID_SOCK)
		sock->s = s;
	else
	{
		free(sock);
		return NULL;
	}
	return sock;
}