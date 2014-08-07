#include "kn_connector.h"
#include "kn_datasocket.h"
#include "kn_proactor.h"
#include "kn_time.h"

static void kn_connector_destroy(void *ptr){
	kn_connector_t c = (kn_connector_t)((char*)ptr - sizeof(kn_dlist_node));
	kn_dlist_remove((kn_dlist_node*)c);
	if(c->base.fd >= 0) close(c->base.fd);
	free(c);
}

static void kn_connector_on_active(struct kn_fd *s,int event){
	kn_connector_t c = (kn_connector_t)s;
	int err = 0;
    socklen_t len = sizeof(err);
    struct kn_sockaddr local;
    kn_fd_t    datasock;
    if (getsockopt(s->fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1) {
        c->cb_connected(NULL,&c->remote,s->ud,err);
        kn_closefd(s);
        return;
    }
    if(err){
        errno = err;
        c->cb_connected(NULL,&c->remote,s->ud,errno);
        kn_closefd(s);
        return;
    }
    //connect success
	c->base.proactor->UnRegister(c->base.proactor,(kn_fd_t)c);   
	local.addrtype = c->remote.addrtype;
	if(local.addrtype == AF_INET){
		len = sizeof(local.in);
		getsockname(s->fd,(struct sockaddr*)&local.in,&len);
	}else if(local.addrtype == AF_INET6){
		len = sizeof(local.in6);
		getsockname(s->fd,(struct sockaddr*)&local.in6,&len);
	}else{
		len = sizeof(local.un);
		getsockname(s->fd,(struct sockaddr*)&local.un,&len);
	}
    datasock = kn_new_datasocket(s->fd,STREAM_SOCKET,&local,&c->remote);
    s->fd = -1;
    c->cb_connected(datasock,&c->remote,s->ud,0);
	kn_closefd(s);
}

kn_connector_t kn_new_connector(int fd,
								struct kn_sockaddr *remote,
								void (*cb_connect)(kn_fd_t,struct kn_sockaddr*,void*,int),
								void *ud)
{
	kn_connector_t c = calloc(1,sizeof(*c));
	c->base.fd = fd;
	kn_ref_init(&c->base.ref,kn_connector_destroy);
	c->base.type = CONNECTOR;
	c->base.on_active = kn_connector_on_active;
	c->base.ud = ud;
	c->remote = *remote;
	c->cb_connected = cb_connect;
	return c;
}



