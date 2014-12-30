#include "stream_conn.h"
#include "kendynet.h"

enum{
	UT_CONN,
	UT_SOCK,
};

typedef struct{
	uint8_t type;
	union{
		stream_conn_t conn;
		handle_t            sock;
	};
}*ut_socket_t;


ut_socket_t ut_accept(ut_socket_t);
int                ut_connect(ut_socket_t,kn_sockaddr *remote);
packet_t      ut_recv(ut_socket_t);
int                ut_send(ut_socket_t);
int                ut_close(ut_socket_t);
