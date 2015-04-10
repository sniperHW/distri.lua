#include "rpacket.h"
#include "wpacket.h"


int main(){
	wpacket_t wpk = wpk_create(64);
	int i = 0;
	for(; i < 64; ++i){
		wpk_write_uint32(wpk,i);
	}
	rpacket_t rpk = (rpacket_t)make_readpacket((packet_t)wpk);
	destroy_packet(wpk);
	printf("here\n");
	for(i = 0; i < 64; ++i){
		printf("%u\n",rpk_read_uint32(rpk));
	}
	destroy_packet(rpk);
	return 0;
}