#ifndef _KN_STREAM_SOCKET_H
#define _KN_STREAM_SOCKET_H

#include "kn_socket.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct{
	kn_socket socket;
	//void   (*cb_accept)(handle_t,void *ud);
	//void   (*cb_connect)(handle_t,int,void *ud,kn_sockaddr*);
	SSL_CTX *ctx;
	SSL *ssl;
}kn_stream_socket;

handle_t new_stream_socket(int fd,int domain);

int stream_socket_close(handle_t h);

int stream_socket_listen(kn_stream_socket*,kn_sockaddr*);

int stream_socket_connect(kn_stream_socket*,kn_sockaddr*,kn_sockaddr*);


#endif