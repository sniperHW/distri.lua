#include "ut_socket.h"

static engine_t g_engine = NULL;

int ut_socket_init(engine_t e){
	if(g_engine || !e) return -1;
	g_engine = e;
}

/*ut_socket_t ut_socket_new(handle_t sock){
	ut_socket_t ut_socket = calloc(1,sizeof(*ut_socket));
	ut_socket->type = UT_SOCK;
	ut_socket->sock = sock;
	return ut_socket;
}*/

int                ut_socket_init(engine_t);
ut_socket_t ut_socket_listen(kn_sockaddr *local);
ut_socket_t ut_accept(ut_socket_t);
ut_socket_t ut_connect(kn_sockaddr *remote);
packet_t      ut_recv(ut_socket_t);
int                ut_send(ut_socket_t);
int                ut_close(ut_socket_t);