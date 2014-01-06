#include "lsock.h"

void luasock_destroy(void *ptr)
{
    _lsock_t sock = (_lsock_t)ptr;
    if(sock->c)
        release_conn(sock->c);
    else if(sock->s != INVALID_SOCK)
        CloseSocket(sock->s);
    free(sock);
}

_lsock_t luasock_new(struct connection *c)
{
    _lsock_t sock = calloc(1,sizeof(*sock));
    ref_init(&sock->ref,luasock_destroy,1);
    if(c){
        sock->c = c;
        c->usr_ptr = sock;
    }
    return sock;
}
