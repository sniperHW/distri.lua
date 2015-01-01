#include "stream_conn.h"
#include "kendynet.h"

struct ut_socket;
typedef struct ut_socket* ut_socket_t;

int                ut_socket_init(engine_t);
ut_socket_t ut_socket_listen(kn_sockaddr *local);
ut_socket_t ut_accept(ut_socket_t);
ut_socket_t ut_connect(kn_sockaddr *remote);
packet_t      ut_recv(ut_socket_t);
int                ut_send(ut_socket_t);
int                ut_close(ut_socket_t);
