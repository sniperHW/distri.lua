CFLAGS = -g -Wall 
LDFLAGS = -lpthread -lrt /usr/lib/libtcmalloc.so.4
SHARED = -fPIC --shared
CC = gcc
INCLUDE = -I../kendynet -I../kendynet/core -I..
DEFINE = -D_DEBUG -D_LINUX -DMQ_HEART_BEAT
TESTDIR = ../kendynet/test

kendynet.a: \
		   ../kendynet/core/src/buffer.c \
		   ../kendynet/core/src/connection.c \
		   ../kendynet/core/src/poller.c \
		   ../kendynet/core/src/epoll.c \
		   ../kendynet/core/src/except.c \
		   ../kendynet/core/src/kendynet.c \
		   ../kendynet/core/src/msgque.c \
		   ../kendynet/core/src/netservice.c \
		   ../kendynet/core/src/rbtree.c \
		   ../kendynet/core/src/rpacket.c \
		   ../kendynet/core/src/socket.c \
		   ../kendynet/core/src/sock_util.c \
		   ../kendynet/core/src/spinlock.c \
		   ../kendynet/core/src/systime.c \
		   ../kendynet/core/src/thread.c \
		   ../kendynet/core/src/timer.c \
		   ../kendynet/core/src/uthread.c \
		   ../kendynet/core/src/refbase.c \
		   ../kendynet/core/src/asynnet.c \
		   ../kendynet/core/src/asynsock.c \
		   ../kendynet/core/src/tls.c \
		   ../kendynet/core/src/wpacket.c
		$(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
		ar -rc kendynet.a *.o
		rm -f *.o

nodelua:nodelua.c lsock.c kendynet.a
	$(CC) $(CFLAGS) -c $(SHARED) nodelua.c lsock.c $(INCLUDE) $(DEFINE) 
	$(CC) $(SHARED) -o nodelua.so nodelua.o lsock.o kendynet.a $(LDFLAGS) $(DEFINE)
	rm -f *.o	
	
	
	
	
	
