#include "superservice/superservice.h"
#include "battleservice/battleservice.h"

static volatile int8_t stop = 0;

static void stop_handler(int signo){
    stop = 1;
}

void setup_signal_handler()
{
	struct sigaction act;
    bzero(&act, sizeof(act));
    act.sa_handler = stop_handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
}


int main()
{
	g_superservice = create_superservice();
	//创建battleservice

	while(stop == 0)
		sleep(1);

	destroy_superservice(&g_superservice);
	//关闭battleservice

	return 0;
}