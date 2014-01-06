#ifndef _LSOCK_H
#define _LSOCK_H
#include "core/connection.h"
#include "core/refbase.h"
#include "core/netservice.h"
#include "core/dlist.h"
#include "core/msgque.h"
#include "lua_util.h"

typedef struct _lsock
{
    struct refbase          ref;
    struct dnode            dn;
    struct connection       *c;
    SOCK                     s;
    luaObject_t             luasocket;
}*_lsock_t;


_lsock_t luasock_new(struct connection *c);

static inline atomic_32_t luasock_release(_lsock_t sock)
{
    return ref_decrease((struct refbase*)sock);
}

static inline atomic_32_t luasock_acquire(_lsock_t sock)
{
    return ref_increase((struct refbase*)sock);
}


#endif
