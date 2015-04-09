#include "connection.h"
#include "kendynet.h"
#include "kn_refobj.h"

//struct ut_socket;
//typedef struct ut_socket* ut_socket_t;
typedef ident ut_socket_t;

int                ut_socket_init(engine_t);
ut_socket_t ut_socket_listen(kn_sockaddr *local);
ut_socket_t ut_accept(ut_socket_t,uint32_t buffersize,decoder*);
ut_socket_t ut_connect(kn_sockaddr *remote,uint32_t buffersize,decoder*);
int                ut_recv(ut_socket_t,packet_t*,int *err);
int                ut_send(ut_socket_t,packet_t);
int                ut_close(ut_socket_t);
void             ut_socket_run();
