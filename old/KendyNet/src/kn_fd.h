#ifndef _KN_FD_H
#define _KN_FD_H

#include <stdint.h>
#include "kn_common_include.h"
#include "kn_ref.h"
#include "kn_dlist.h"
#include "kn_sockaddr.h"
#include "kn_common_define.h"


enum{
	readable  = 1,
	writeable = 1 << 1,
};

struct kn_proactor;
typedef struct kn_fd{
	kn_dlist_node       node;
    kn_ref              ref;
	uint8_t             type;
	int32_t             fd;
	void*               ud;
	struct kn_proactor* proactor;
	int                 events;		
	void (*on_active)(struct kn_fd*,int event);
	int8_t (*process)(struct kn_fd*);//如果process返回非0,则需要重新投入到activedlist中
}kn_fd,*kn_fd_t;

void kn_closefd(kn_fd_t);

void kn_fd_addref(kn_fd_t fd);

void kn_fd_subref(kn_fd_t fd);

#endif
