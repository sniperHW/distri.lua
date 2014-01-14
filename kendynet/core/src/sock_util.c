#include "sock_util.h"
#include "common.h"
#include "socket.h"
#include "poller.h"

typedef int32_t SOCKET;

static void InitSocket(SOCK sock,SOCKET fd)
{
	socket_t s = get_socket_wrapper(sock);
	s->fd = fd;
	s->readable = s->writeable = 0;
	s->engine = NULL;
}


SOCK OpenSocket(int32_t family,int32_t type,int32_t protocol)
{
	SOCKET sockfd;
	if( (sockfd = socket(family,type,protocol)) < 0)
	{
		return INVALID_SOCK;
	}
	SOCK sock = new_socket_wrapper();
	if(sock == INVALID_SOCK)
	{
		close(sockfd);
		return INVALID_SOCK;
	}
	InitSocket(sock,sockfd);
	return sock;
}


void    ShutDownRecv(SOCK sock)
{
	struct socket_wrapper *sw = get_socket_wrapper(sock);
	if(sw)shutdown_recv(sw);
}

void    ShutDownSend(SOCK sock)
{
	struct socket_wrapper *sw = get_socket_wrapper(sock);
	if(sw)shutdown_send(sw);
}

int32_t CloseSocket(SOCK sock)
{
	struct socket_wrapper *sw = get_socket_wrapper(sock);
    if(sw){ 
		if(sw->engine) sw->engine->UnRegister(sw->engine,sw);
		close(sw->fd);
		release_socket_wrapper(sock);
		return 0;
	}
	return -1;
}

SOCK Connect(SOCK sock,const struct sockaddr *servaddr,socklen_t addrlen)
{
	socket_t s = get_socket_wrapper(sock);
	if(s)
	{
		if(connect(s->fd,servaddr,addrlen) < 0)
		{
			//printf("%s\n",strerror(errno));
			return INVALID_SOCK;
		}
		return sock;
	}
	return INVALID_SOCK;

}

SOCK Tcp_Connect(const char *ip,uint16_t port)
{
	if(!ip)
		return INVALID_SOCK;

    struct sockaddr_in servaddr;
	memset((void*)&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = INET;
	servaddr.sin_port = htons(port);
	if(inet_pton(INET,ip,&servaddr.sin_addr) < 0)
	{
		printf("%s\n",strerror(errno));
		return INVALID_SOCK;
	}
	SOCK sock = OpenSocket(INET,STREAM,TCP);
	if(sock != INVALID_SOCK)
	{
        if(INVALID_SOCK != Connect(sock,(struct sockaddr*)&servaddr,sizeof(servaddr)))
            return sock;
		CloseSocket(sock);
	}
	return INVALID_SOCK;
}

SOCK Bind(SOCK sock,const struct sockaddr *myaddr,socklen_t addrlen)
{
	socket_t s = get_socket_wrapper(sock);
	if(s)
	{
		if(bind(s->fd,myaddr,addrlen) < 0)
		{
			printf("%s\n",strerror(errno));
			return INVALID_SOCK;
		}
		return sock;
	}
	return INVALID_SOCK;
}

SOCK Listen(SOCK sock,int32_t backlog)
{
	socket_t s = get_socket_wrapper(sock);
	if(s)
	{
		if(listen(s->fd,backlog) < 0)
		{
			printf("%s\n",strerror(errno));
			return INVALID_SOCK;
		}
		return sock;
	}
	return INVALID_SOCK;
}

SOCK Tcp_Listen(const char *ip,uint16_t port,int32_t backlog)
{
    struct sockaddr_in servaddr;
	SOCK sock;
	sock = OpenSocket(INET,STREAM,TCP);
	if(sock == INVALID_SOCK)
		return INVALID_SOCK;

	memset((void*)&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = INET;
	if(ip)
	{
		if(inet_pton(INET,ip,&servaddr.sin_addr) < 0)
		{

			printf("%s\n",strerror(errno));
			return INVALID_SOCK;
		}
	}
	else
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	if(Bind(sock,(struct sockaddr*)&servaddr,sizeof(servaddr)) == INVALID_SOCK)
	{
		CloseSocket(sock);
		return INVALID_SOCK;
	}

	if(Listen(sock,backlog) != INVALID_SOCK)
		return sock;
	else
	{
		CloseSocket(sock);
		return INVALID_SOCK;
	}
}


SOCK _accept(socket_t s,struct sockaddr *sa,socklen_t *salen)
{
	if(s)
	{
		int32_t n;
	again:
		if((n = accept(s->fd,sa,salen)) < 0)
		{
	#ifdef EPROTO
			if(errno == EPROTO || errno == ECONNABORTED)
	#else
			if(errno == ECONNABORTED)
	#endif
				goto again;
			else
			{
				//printf("%s\n",strerror(errno));
				return INVALID_SOCK;
			}
		}
		SOCK newsock = new_socket_wrapper();
		if(newsock == INVALID_SOCK)
		{
			close(n);
			return INVALID_SOCK;
		}
		InitSocket(newsock,n);
		return newsock;
	}
	return INVALID_SOCK;
}

/*
int32_t getLocalAddrPort(SOCK sock,struct sockaddr_in *remoAddr,socklen_t *len,char *buf,uint16_t *port)
{

	socket_t s = get_socket_wrapper(sock);
	if(s)
	{
		if(0 == buf)
			return -1;
		int32_t ret = getsockname(s->fd, (struct sockaddr*)remoAddr,len);
		if(ret != 0)
			return -1;
		if(0 == inet_ntop(INET,&remoAddr->sin_addr,buf,15))
			return -1;
		*port = ntohs(remoAddr->sin_port);
		return 0;
	}
	return -1;
}


int32_t getRemoteAddrPort(SOCK sock,char *buf,uint16_t *port)
{
	socket_t s = get_socket_wrapper(sock);
	if(s)
	{
		if(0 == buf)
			return -1;
		struct sockaddr_in remoAddr;
		memset(&remoAddr,0,sizeof(remoAddr));
		remoAddr.sin_family = INET;
		socklen_t len = sizeof(remoAddr);
		int32_t ret = getpeername(s->fd,(struct sockaddr*)&remoAddr,&len);
		if(ret != 0)
			return -1;
		if(0 == inet_ntop(INET,&remoAddr.sin_addr,buf,15))
			return -1;
		*port = ntohs(remoAddr.sin_port);
		return 0;
	}
	return -1;
}
*/
int32_t setNonblock(SOCK sock)
{

	socket_t s = get_socket_wrapper(sock);
	if(s)
	{
		int ret;
		ret = fcntl(s->fd, F_SETFL, O_NONBLOCK | O_RDWR);
		if (ret != 0) {
			return -1;
		}
		return 0;
	}
	return -1;
}

/*
struct hostent *Gethostbyaddr(const char *ip,int32_t family)
{

	if(!ip)
		return NULL;
	struct sockaddr_in servaddr;
	struct hostent	*hptr;

	memset((void*)&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
#ifdef _WIN
	servaddr.sin_addr.s_addr = inet_addr(ip);
#else
	if(inet_pton(INET,ip,&servaddr.sin_addr) < 0)
	{

		printf("%s\n",strerror(errno));
		return NULL;
	}
#endif

	if ( (hptr = gethostbyaddr((const char*)&servaddr.sin_addr,sizeof(servaddr.sin_addr),family)) == NULL) {
		return NULL;
	}

	return hptr;
}
*/
