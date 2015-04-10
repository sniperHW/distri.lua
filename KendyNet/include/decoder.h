#ifndef _DECODER_H
#define _DECODER_H

#define MAX_WBAF 512
#define MAX_SEND_SIZE 65535

enum {
	decode_packet_too_big = -1,
	decode_socket_close = -2,
	decode_unknow_error = -3,
};

struct connection;
//解包器接口
typedef struct decoder{
	int  (*unpack)(struct decoder*,void*);
	void (*destroy)(struct decoder*);
	int  mask;
}decoder;

void destroy_decoder(decoder*);

#endif