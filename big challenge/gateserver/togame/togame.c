#include "togame.h"

toGame_t g_togame = NULL;

void send2game(wpacket_t wpk)
{
	asyn_send(g_togame->togame,wpk);
}