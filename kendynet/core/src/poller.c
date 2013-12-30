#include "kendynet.h"
#include "llist.h"
#include <stdlib.h>
#include <assert.h>
#include "common.h"
#include "poller.h"
#include "epoll.h"

poller_t poller_new()
{
    poller_t e = malloc(sizeof(*e));
	if(e)
	{
		e->Init = epoll_init;
		e->Loop = epoll_loop;
		e->Register = epoll_register;
		e->UnRegister = epoll_unregister;
        e->UnRegisterRecv = epoll_unregister_recv;
        e->UnRegisterSend = epoll_unregister_send;
		e->WakeUp = epoll_wakeup;
        e->actived_index = 0;
        dlist_init(&e->actived[0]);
        dlist_init(&e->actived[1]);
        dlist_init(&e->connecting);
	}
	return e;
}

void   poller_delete(poller_t *e)
{
	assert(e);
	assert(*e);
	free(*e);
	*e = 0;
}
