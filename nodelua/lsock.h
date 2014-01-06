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

static inline void luasock_release(_lsock_t sock)
{
    ref_decrease((struct refbase*)sock);
}


#endif
