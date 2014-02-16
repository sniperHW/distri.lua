#define MAX_CLIENT 2000
static struct connection *clients[MAX_CLIENT];

static int packet_send_count = 0;
static int client_count = 0;

void init_clients()
{
	uint32_t i = 0;
	for(; i < MAX_CLIENT;++i)
		clients[i] = 0;
}

void add_client(struct connection *c)
{
	uint32_t i = 0;
	for(; i < MAX_CLIENT; ++i)
	{
		if(clients[i] == 0)
		{
			clients[i] = c;
			++client_count;
			break;
		}
	}
}

void send2_all_client(rpacket_t r)
{
	uint32_t i = 0;
	for(; i < MAX_CLIENT; ++i){
		if(clients[i]){
            send_packet(clients[i],wpk_create_by_rpacket(r));
			++packet_send_count;
		}
	}
}

void remove_client(struct connection *c,uint32_t reason)
{
	printf("client disconnect,reason:%u\n",reason);
	uint32_t i = 0;
	for(; i < MAX_CLIENT; ++i){
		if(clients[i] == c){
			clients[i] = 0;
			--client_count;
			break;
		}
	}
	release_conn(c);
}

void sendpacket()
{
	uint32_t i = 0;
	for(; i < MAX_CLIENT; ++i){
		if(clients[i]){
            wpacket_t wpk = NEW_WPK(64);
            wpk_write_uint32(wpk,(uint32_t)clients[i]);
            uint32_t sys_t = GetSystemMs();
            wpk_write_uint32(wpk,sys_t);
            wpk_write_string(wpk,"hello kenny");
            send_packet(clients[i],wpk);
		}
	}
}


static volatile int8_t stop = 0;

static void stop_handler(int signo){
	printf("stop_handler\n");
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

void c_recv_timeout(struct connection *c)
{
	printf("recv timeout\n");
	active_close(c);
}

void c_send_timeout(struct connection *c)
{
	printf("send timeout\n");
	active_close(c);
}
