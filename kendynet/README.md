#【kendynet高效的C网络通信框架】#

kendynet是一个用C语言打造的跨平台的网络通信框架（目前只实现了对Linux epoll的支持），其设计目标是让使用者能方便快捷的构建自己的网络应用.kendynet目前只支持TCP通信方式，对UDP，SSL的支持将会在后续版本中考虑.

pingpong测试，测试环境为intel 酷睿2.53hz 的双核笔记本，运行ubuntu 12.4系统
不加任何优化编译,客户端与服务器均为单线程，运行在同一台机器.

	发送字节数    连接数      吞吐量/s
	4k             1        180M
	4k             10       440M
	4k             100      320M
	4k             1000     260M

	发送字节数    连接数      吞吐量/s
	16k            1        460M
	16k            10       990M
	16k            100      580M
	16K            1000     570M

kendynet内置了3层接口API，使用者可以根据自己的使用需求选用不同的API,也可以在这些API之上打造自己的框架.

##【第一层API】##

kendynet的网络处理模式模仿了iocp,与传统的send,recv调用不同，kendynet发送和接收接口要提供的不仅仅是一个缓冲区,而是一个叫做st_io的结构:

	/*IO请求和完成队列使用的结构*/
	typedef struct
	{
    	lnode      next;
		struct     iovec *iovec;
		int32_t    iovec_count;
	}st_io;


这个结构封装了IO请求需要的缓冲区，当对应的请求完成时通过注册的回调函数返回给使用者.

##【API说明】##

`int32_t InitNetSystem();`

`void   CleanNetSystem();`

初始化和清理网络系统，分别在主程序的开始和结束时调用.

`ENGINE   CreateEngine();`

`void     CloseEngine(ENGINE);`

创建和关闭一个网络引擎.

`int32_t  EngineRun(ENGINE,int32_t timeout);`

驱动网络引擎，当没有事件处理时，最多在函数内等待timeout毫秒

`int32_t  Bind2Engine(ENGINE,SOCK,OnIoFinish,OnClearPending);`

将一个套接字与一个引擎绑定,当套接字上有完成的发送/接收请求时，回调用户传入的
OnIoFinish函数,在传入回调函数的参数中表明了拿个IO请求被完成了.当套接字被关闭
时,如果还有用户的IO请求没有被完成,用用户传入的OnClearPending函数处理那些没有
完成的请求.


`SOCK EListen(ENGINE,const char *ip,int32_t port,void*ud,OnAccept);`

添加对ip/port的监听，并返回一个SOCK,当用户想关闭监听时直接对那个SOCK执行CloseSocket即可.
当由连接到达时，回掉用户提供的OnAccept函数,并传回给用户ud参数.


`int32_t EConnect(ENGINE,const char *ip,int32_t port,void*ud,OnConnect,uint32_t ms);`

建立到ip/port的连接，最多等待ms毫秒.函数是非阻塞的,当连接建立/失败或超时都会回调用户
传入的OnConnect函数，并将ud参数传回给用户.


`int32_t Recv(SOCK,st_io*,uint32_t *err_code);`

`int32_t Send(SOCK,st_io*,uint32_t *err_code);`

发起一个发送/接收请求，如果请求立即可以完成函数返回完成的字节数，如果请求无法立即完成
返回0,当请求在将来的某个时刻完成时回调调用Bind2Engine时传递的进去的OnIoFinish函数.


`int32_t Post_Recv(SOCK,st_io*);`

`int32_t Post_Send(SOCK,st_io*);`

投递一个发送/接收请求之后立即返回,当IO被完成时回调调用Bind2Engine时传递的进去的OnIoFinish
函数.


##【第二层API】##

在第一层的基础增加了connection,wpacket,rpacket用于处理连接，网络封包等。wpacket,rpacket
的底层使用buffer list做为存储结构，通过缓冲区重用有效的减少了内存拷贝的消耗.


##【第三层API】##

前面两层的接口都是单线程的，使用者只能在单个线程中使用.在第三层中，提供了一个多线程的，网络处理引擎和消息分发器分离的网络服务框架。

通过将网络和消息分发处理器的分离，让使用者可以更加灵活的搭建应用.例如：可以创建一个消息分发器，然后让多个线程
等待在上面，这就是典型的线程池处理消息的模型.然后，还可以启动多个线程，每个线程运行一个不同的服务，创建自己单独的消息分发器，让只属于本服务的消息路由到正确的处理逻辑中.


下面是一个多服务共用网络层的模式，创建了两个线程各运行一个echo服务，一个监听8010端口，一个监听8011端口，
客户端连接到不同的端口，将由不同的服务实例为其提供服务.

	#include <stdio.h>
	#include <stdlib.h>
	#include "core/msgdisp.h"
	#include "testcommon.h"
	
	uint32_t recvsize = 0;
	uint32_t recvcount = 0;
	extern int disconnect_count;
	
	void asynconnect(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port)
	{
	    printf("asynconnect,ip:%s,port:%d\n",ip,port);
	    disp->bind(disp,0,sock,1,3*1000,0);//由系统选择poller
	}
	
	void asynconnected(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port)
	{
	    printf("asynconnected\n");
	   ++client_count;
	}
	
	void asyndisconnected(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port,uint32_t err)
	{
	    --client_count;
	}
	
	
	int32_t asynprocesspacket(msgdisp_t disp,sock_ident sock,rpacket_t rpk)
	{
	    recvsize += rpk_len(rpk);
	    recvcount++;
	    asyn_send(sock,wpk_create_by_other((struct packet*)rpk));
	    return 1;
	}
	
	
	void asynconnectfailed(msgdisp_t disp,const char *ip,int32_t port,uint32_t reason)
	{
	
	}
	
	
	static const char *ip;
	static int32_t port;
	
	static void *service_main(void *ud){
	
	    printf("echo service port:%d\n",port);
	    msgdisp_t disp = (msgdisp_t)ud;
	    int32_t err = 0;
	    disp->listen(disp,0,ip,port++,&err);
	    while(!stop){
	        msg_loop(disp,50);
	    }
	    return NULL;
	}
	
	
	int main(int argc,char **argv)
	{
	    setup_signal_handler();
	    InitNetSystem();
	
	    //共用网络层，两个线程各运行一个echo服务
	
	    asynnet_t asynet = asynnet_new(1);
	    msgdisp_t  disp1 = new_msgdisp(asynet,
	                                  asynconnect,
	                                  asynconnected,
	                                  asyndisconnected,
	                                  asynprocesspacket,
	                                  asynconnectfailed);
	
	    msgdisp_t  disp2 = new_msgdisp(asynet,
	                                  asynconnect,
	                                  asynconnected,
	                                  asyndisconnected,
	                                  asynprocesspacket,
	                                  asynconnectfailed);
	
	    thread_t service1 = create_thread(THREAD_JOINABLE);
	    thread_t service2 = create_thread(THREAD_JOINABLE);
	
	    ip = argv[1];
	    port = atoi(argv[2]);
	
	    thread_start_run(service1,service_main,(void*)disp1);
	    sleepms(1000);
	    thread_start_run(service2,service_main,(void*)disp2);
	
	    uint32_t tick,now;
	    tick = now = GetSystemMs();
	    while(!stop){
	        sleepms(100);
	        now = GetSystemMs();
	        if(now - tick > 1000)
	        {
	            uint32_t elapse = now-tick;
	            recvsize = (recvsize/elapse)/1000;
	            printf("client_count:%d,recvsize:%d,recvcount:%d,discount:%d\n",client_count,recvsize,recvcount,disconnect_count);
	            tick = now;
	            packet_send_count = 0;
	            recvcount = 0;
	            recvsize = 0;
	        }
	    }
	
	    thread_join(service1);
	    thread_join(service2);
	
	    CleanNetSystem();
	    return 0;
	}


