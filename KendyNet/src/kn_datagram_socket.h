#ifndef _KN_DATAGRAM_SOCKET_H
#define _KN_DATAGRAM_SOCKET_H

#include "kn_socket.h"

typedef struct{
	kn_socket socket;
	uint8_t       connected;
}kn_datagram_socket;

handle_t new_datagram_socket(int fd,int domain);

int datagram_socket_close(handle_t h);

int datagram_socket_listen(kn_datagram_socket*,kn_sockaddr*);

int datagram_socket_connect(kn_datagram_socket*,kn_sockaddr*,kn_sockaddr*);

#endif