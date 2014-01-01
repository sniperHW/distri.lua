【kendylib的网络模块】

将kendylib中的网络模块单独分离出来，重新提炼了接口,调整实现，将原来单独实现的Acceptor和Connector
统一进Engine中，通过Engine提供的接口即可实现异步的accpet和connect.

本库致力于向用户提供最简单/基本的网络接口函数，让使用者可以方便的编写简单的异步网络程序，所以向用户
提供了两层抽象接口，第一层接口向使用者提供了3个类型和一组API,只提供了异步的accpet,connect,recv和send
等几个功能，不涉及网络封包，定时器的处理等问题.使用者可在其上封装出满足自己需求的上层接口和框架.

第二层抽象封装了第一层的类型和API,引出了connection和wpacket,rpacket等，其中wpacket,rpacket是由buff list
实现的基于字节流的封包。

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



【抽象层次1】

typedef struct
{
	LIST_NODE;
	struct     iovec *iovec;
	int32_t    iovec_count;
}st_io;

抽象了一个网络IO的发送/接收请求,iovec,iovec_count由用户负责填充，表示用于接收或希望发送出去的缓冲.
IO请求支持gather recv/send,所以用户可以在一个请求中输入多个缓冲区.


ENGINE
网络引擎，底层实现与平台相关，在linux下是epoll,windows下是iocp.使用者通过驱动ENGINE完成网络的事件循环.


SOCK
套接字的抽象，目前只支持TCP协议.

【API说明】

int32_t InitNetSystem();

void   CleanNetSystem();

初始化和清理网络系统，分别在主程序的开始和结束时调用.

ENGINE   CreateEngine();
void     CloseEngine(ENGINE);

创建和关闭一个网络引擎.


int32_t  EngineRun(ENGINE,int32_t timeout);

驱动网络引擎，当没有事件处理时，最多在函数内等待timeout毫秒

int32_t  Bind2Engine(ENGINE,SOCK,OnIoFinish,OnClearPending);

将一个套接字与一个引擎绑定,当套接字上有完成的发送/接收请求时，回调用户传入的
OnIoFinish函数,在传入回调函数的参数中表明了拿个IO请求被完成了.当套接字被关闭
时,如果还有用户的IO请求没有被完成,用用户传入的OnClearPending函数处理那些没有
完成的请求.


SOCK EListen(ENGINE,const char *ip,int32_t port,void*ud,OnAccept);
添加对ip/port的监听，并返回一个SOCK,当用户想关闭监听时直接对那个SOCK执行CloseSocket即可.
当由连接到达时，回掉用户提供的OnAccept函数,并传回给用户ud参数.


int32_t EConnect(ENGINE,const char *ip,int32_t port,void*ud,OnConnect,uint32_t ms);
建立到ip/port的连接，最多等待ms毫秒.函数是非阻塞的,当连接建立/失败或超时都会回调用户
传入的OnConnect函数，并将ud参数传回给用户.


int32_t Recv(SOCK,st_io*,uint32_t *err_code);
int32_t Send(SOCK,st_io*,uint32_t *err_code);

发起一个发送/接收请求，如果请求立即可以完成函数返回完成的字节数，如果请求无法立即完成
返回0,当请求在将来的某个时刻完成时回调调用Bind2Engine时传递的进去的OnIoFinish函数.


int32_t Post_Recv(SOCK,st_io*);
int32_t Post_Send(SOCK,st_io*);

投递一个发送/接收请求之后立即返回,当IO被完成时回调调用Bind2Engine时传递的进去的OnIoFinish
函数.


下面代码展示了一个简单的echo服务器：

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "core/KendyNet.h"

enum{
    io_recv = 1,
    io_send = 2,
};

struct _con{
    st_io st;
    int8_t current_io_type;
    SOCK sock;
    struct iovec wbuf[1];
    char buf[1024];
};

#define PREPARE_BUFFER(SIZE)\
    con->wbuf[0].iov_len = SIZE;\
	con->wbuf[0].iov_base = con->buf;\
	con->st.iovec_count = 1;\
	con->st.iovec = con->wbuf;

#define CHECK_CONNECTION\
    struct _con *con = (struct _con*)st;\
    if(bytes < 0 && err_code != EAGAIN){\
        close_con(con);\
        return;\
    }

int32_t client_count = 0;

void close_con(struct _con *con)
{
    CloseSocket(con->sock);
    free(con);
    printf("client_count:%d\n",--client_count);
}

void con_recv(struct _con *con)
{
    PREPARE_BUFFER(1024);
    con->current_io_type = io_recv;
    Post_Recv(con->sock,(st_io*)con);
}

void con_send(struct _con *con,int32_t bytes)
{
    PREPARE_BUFFER(bytes);
    con->current_io_type = io_send;
    Post_Send(con->sock,(st_io*)con);
}

void on_io_finish(int32_t bytes,st_io *st,uint32_t err_code)
{
    CHECK_CONNECTION;
    if(con->current_io_type == io_recv)
        con_send(con,bytes);
    else
        con_recv(con);
}

void accept_client(SOCK s,void*ud)
{
    struct _con *con = malloc(sizeof(*con));
    con->sock = s;
    Bind2Engine((ENGINE)ud,s,on_io_finish,NULL);
    con_recv(con);
    printf("client_count:%d\n",++client_count);
}

static volatile int8_t stop = 0;

static void stop_handler(int signo){
    stop = 1;
}

int main(int argc,char **argv)
{
    struct sigaction act;
    bzero(&act, sizeof(act));
    act.sa_handler = stop_handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    InitNetSystem();
    ENGINE engine = CreateEngine();
    EListen(engine,argv[1],atoi(argv[2]),(void*)engine,accept_client);
    while(!stop){
        EngineRun(engine,100);
    }
    CleanNetSystem();
    return 0;
}

【抽象层次2】

wpacket和rpacket

wpacket和rpacket是kendylib中提供的基于字节流的封包处理结构,底层缓冲由buff list
管理.最初的设计目的是为了处理网游网关服务器频繁的广播。

例如：从场景服务其中收到一个玩家的移动包，需要广播给网关服务器中另外1000个玩家.

因为各packet可以共享底层的buff list,而有各自的读写指针.所以只需要创建1000个wpacket
用接收到的rpacket的buff list去初始化这1000个wpacket,有效的减少了缓冲被复制的次数.


基于connection的广播服务器

#include <stdio.h>
#include <stdlib.h>
#include "core/KendyNet.h"
#include "core/Connection.h"

#define MAX_CLIENT 2000
static struct connection *clients[MAX_CLIENT];

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
			break;
		}
	}
}

void send2_all_client(rpacket_t r)
{
	uint32_t i = 0;
	for(; i < MAX_CLIENT; ++i){
		if(clients[i]){
			send_packet(clients[i],wpk_create_by_packet(r),NULL);
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
			break;
		}
	}
}

void on_process_packet(struct connection *c,rpacket_t r)
{
	send2_all_client(r);
}

void accept_client(SOCK s,void*ud)
{
	struct connection *c = new_conn(s,0,on_process_packet,remove_client);
	add_client(c);
	bind2engine((ENGINE)ud,c);
}

static volatile int8_t stop = 0;

static void stop_handler(int signo){
    stop = 1;
}

int main(int argc,char **argv)
{
    struct sigaction act;
    bzero(&act, sizeof(act));
    act.sa_handler = stop_handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
	init_clients();
    InitNetSystem();
    ENGINE engine = CreateEngine();
    EListen(engine,argv[1],atoi(argv[2]),(void*)engine,accept_client);
	while(1){
		EngineRun(engine,50);
	}
    CleanNetSystem();
    return 0;
}
