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
    struct    iovec wrecvbuf[2];
    st_io      recv_overlap;
    void*    ud;
    void      (*destroy_ud)(void*);
    uint32_t next_recv_pos;
    buffer_t next_recv_buf;
    //buffer_t unpack_buf;
    decoder* _decoder;d
    uint32_t recv_bufsize;
    void     (*on_packet)(struct datagram*,packet_t,kn_sockaddr);
    uint8_t  doing_recv:1;
    uint8_t  close_step:2;
    uint8_t  processing:1;         
}datagram,*datagram_t;


typedef void  (*DCCB_PROCESS_PKT)(datagram_t,packet_t,kn_sockaddr);

datagram_t new_datagram(handle_t sock,uint32_t buffersize,decoder *_decoder);
void              datagram_close(datagram_t c);
int                 datagram_send(datagram_t c,packet_t,kn_sockaddr*);

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

static inline int datagram_isclose(datagram_t c){
    return c->close_step > 0;
}

int     datagram_associate(engine_t,datagram_t conn,DCCB_PROCESS_PKT);

typedef struct {
    decoder base;
    uint32_t maxpacket_size;
}datagram_rpk_decoder;

typedef struct {
    decoder base;
}datagram_rawpk_decoder;

decoder* new_datagram_rpk_decoder(uint32_t maxpacket_size);
decoder* new_datagram_rawpk_decoder();

#endif