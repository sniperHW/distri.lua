#include "agentservice.h"
#include "core/tls.h"

typedef struct idnode
{
	lnode    node;	
	uint16_t id;
}idnode;


static void *service_main(void *ud){
    agentservice_t service = (agentservice_t)ud;
    tls_create(AGETNSERVICE_TLS,(void*)service,NULL);
    while(!service->stop){
        msg_loop(service->msgdisp,50);
    }
    return NULL;
}


static void agent_connected(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port)
{

}

statci void agent_disconnected(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port,uint32_t err)
{

}


int32_t agent_processpacket(msgdisp_t disp,msgsender sender,rpacket_t rpk)
{
    return 1;
}


agentservice_t new_agentservice(uint8_t agentid,asynnet_t asynet){
	agentservice_t service = calloc(1,sizeof(*service));
	service->agentid = agentid;
	llist_init(&service->idpool);
	int i = 0;
	for(; i < MAX_ANGETPLAYER; ++i){
		idnode *id = calloc(1,sizeof(*id));
		id->id = i;
		LLIST_PUSHBACK(&service->idpool,(lnode*)id);
	}
	service->msgdisp = new_msgdisp(asynet,
                             	   NULL,
                                   agent_connected,
                                   agent_disconnected,
                                   agent_processpacket,
                                   NULL);
	service->thd = create_thread(THREAD_JOINABLE); 
	thread_start_run(service->thd,service_main,(void*)service);
	return service;
}


void destroy_agentservice(agentservice_t s)
{
	//destroy_agentservice应该在gateserver关闭时调用，所以这里不需要做任何释放操作了
	s->stop = 1;
	thread_join(s->thd);
}

agentservice_t get_thd_agentservice()
{
	return (agentservice_t)tls_get(AGETNSERVICE_TLS);
}