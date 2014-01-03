#ifndef _LSOCK_H
#define _LSOCK_H
#include "core/connection.h"
#include "core/refbase.h"
#include "core/netservice.h"
#include "core/dlist.h"
#include "core/msgque.h"

typedef struct luasock{
    ident _ident;
}luasock;

#define CAST_2_LUASOCK(IDENT) (*(luasock*)&IDENT)

typedef struct _lsock
{
    struct refbase          ref;
    struct dnode            dn;
    struct connection       *c;
    SOCK                     s;
    void                    *usr_ptr;
    luasock                 sident;
}*_lsock_t;


_lsock_t luasock_new(struct connection *c);

static inline _lsock_t cast_2_luasock(luasock sock)
{
    ident _ident = TO_IDENT(sock);
    struct refbase *r = cast_2_refbase(_ident);
    if(r) return (_lsock_t)r;
    return NULL;
}

static inline void luasock_release(_lsock_t sock)
{
    ref_decrease((struct refbase*)sock);
}


#endif
