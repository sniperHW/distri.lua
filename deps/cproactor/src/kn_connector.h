#ifndef _KN_CONNECTOR_H
#define _KN_CONNECTOR_H

#include "kn_fd.h"

struct kn_proactor;

typedef struct kn_connector{
	struct kn_fd   base;
	uint64_t           timeout;
	struct kn_sockaddr remote;
	/*
	 * 连接失败或成功回调
	 * 如果成功sock为新建立的套接口,否则sock为NULL,err标识错误信息
	 */
	void (*cb_connected)(kn_fd_t fd,struct kn_sockaddr*,void *ud,int err);
}*kn_connector_t;


kn_connector_t kn_new_connector(int fd,
								struct kn_sockaddr*,
								void (*)(kn_fd_t,struct kn_sockaddr*,void*,int),
								void *ud,
								uint64_t timeout);




#endif
