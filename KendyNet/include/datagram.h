#ifndef _DATAGRAM_H
#define _DATAGRAM_H

#include "kendynet.h"
#include "packet.h"
#include "rpacket.h"
#include "wpacket.h"
#include "rawpacket.h"
#include "kn_refobj.h"
#include "kn_timer.h"
#include "kn_sockaddr.h"
#include "decoder.h"

typedef struct datagram
{
    refobj   refobj;    
    handle_t handle;
    struct    iovec wsendbuf[MAX_WBAF];
    struct    iovec wrecvbuf;
    st_io      recv_overlap;
    void*    ud;
    void      (*destroy_ud)(void*);
    buffer_t recv_buf;    
    decoder* _decoder;
    uint32_t recv_bufsize;
    void     (*on_packet)(struct datagram*,packet_t,kn_sockaddr*);
    uint8_t  doing_recv;         
}datagram,*datagram_t;


typedef void  (*DCCB_PROCESS_PKT)(datagram_t,packet_t,kn_sockaddr*);

datagram_t new_datagram(int domain,uint32_t buffersize,decoder *_decoder);

void              datagram_close(datagram_t c);
int                 datagram_send(datagram_t c,packet_t,kn_sockaddr*);

datagram_t datagram_listen(kn_sockaddr*,uint32_t buffersize,decoder *_decoder);

static inline handle_t datagram_gethandle(datagram_t c){
    return c->handle;
} 

static inline void datagram_setud(datagram_t c,void *ud,void (*destroy_ud)(void*)){
    c->ud = ud;
    c->destroy_ud = destroy_ud;
}
static inline void* datagram_getud(datagram_t c){
    return c->ud;
}

int     datagram_associate(engine_t,datagram_t conn,DCCB_PROCESS_PKT);

typedef struct {
    decoder base;
}datagram_rpk_decoder;

typedef struct {
    decoder base;
}datagram_rawpk_decoder;

decoder* new_datagram_rpk_decoder();
decoder* new_datagram_rawpk_decoder();

#endif