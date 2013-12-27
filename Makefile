CFLAGS = -O2 -g -Wall 
LDFLAGS = -lpthread -lrt -ltcmalloc
SHARED = -fPIC --shared
CC = gcc
INCLUDE = -Ikendynet -Ikendynet/core -I..
DEFINE = -D_DEBUG -D_LINUX -DMQ_HEART_BEAT
TESTDIR = kendynet/test

kendynet.a: \
		   kendynet/core/src/buffer.c \
		   kendynet/core/src/Connection.c \
		   kendynet/core/src/Engine.c \
		   kendynet/core/src/epoll.c \
		   kendynet/core/src/except.c \
		   kendynet/core/src/KendyNet.c \
		   kendynet/core/src/msg_que.c \
		   kendynet/core/src/netservice.c \
		   kendynet/core/src/RBtree.c \
		   kendynet/core/src/rpacket.c \
		   kendynet/core/src/Socket.c \
		   kendynet/core/src/sock_util.c \
		   kendynet/core/src/spinlock.c \
		   kendynet/core/src/SysTime.c \
		   kendynet/core/src/thread.c \
		   kendynet/core/src/timer.c \
		   kendynet/core/src/uthread.c \
		   kendynet/core/src/refbase.c \
		   kendynet/core/src/wpacket.c
		$(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
		ar -rc kendynet.a *.o
		rm -f *.o

luanet:luanet.c kendynet.a
	$(CC) $(CFLAGS) -c $(SHARED) luanet.c $(INCLUDE) $(DEFINE) 
	$(CC) $(SHARED) -o luanet.so luanet.o kendynet.a $(LDFLAGS) $(DEFINE)
	rm -f *.o
	
tcpserver:kendynet.a $(TESTDIR)/benchserver.c $(TESTDIR)/testcommon.h
	$(CC) $(CFLAGS) -o tcpserver $(TESTDIR)/benchserver.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
tcpclient:kendynet.a $(TESTDIR)/benchclient.c $(TESTDIR)/testcommon.h
	$(CC) $(CFLAGS) -o tcpclient $(TESTDIR)/benchclient.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
timer:kendynet.a $(TESTDIR)/testtimer.c
	$(CC) $(CFLAGS) -o timer $(TESTDIR)/testtimer.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)				
msgque:kendynet.a $(TESTDIR)/testmq.c
	$(CC) $(CFLAGS) -o msgque $(TESTDIR)/testmq.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
systick:kendynet.a $(TESTDIR)/testgetsystick.c
	$(CC) $(CFLAGS) -o systick $(TESTDIR)/testgetsystick.c kendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE)
	
	
