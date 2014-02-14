CFLAGS = -O2 -g -Wall 
LDFLAGS = -lpthread -lrt -ltcmalloc -lm
SHARED = -fPIC --shared
CC = gcc
INCLUDE = -Ikendynet -Ikendynet/core -I..\
		  -I/usr/local/include/luajit-2.0 -Ikendynet/deps/hiredis
DEFINE = -D_DEBUG -D_LINUX -DMQ_HEART_BEAT
TESTDIR = kendynet/test

kendynet.a: \
		   kendynet/core/src/buffer.c \
		   kendynet/core/src/connection.c \
		   kendynet/core/src/poller.c \
		   kendynet/core/src/epoll.c \
		   kendynet/core/src/except.c \
		   kendynet/core/src/kendynet.c \
		   kendynet/core/src/msgque.c \
		   kendynet/core/src/netservice.c \
		   kendynet/core/src/rbtree.c \
		   kendynet/core/src/rpacket.c \
		   kendynet/core/src/socket.c \
		   kendynet/core/src/sock_util.c \
		   kendynet/core/src/spinlock.c \
		   kendynet/core/src/systime.c \
		   kendynet/core/src/thread.c \
		   kendynet/core/src/timer.c \
		   kendynet/core/src/uthread.c \
		   kendynet/core/src/refbase.c \
		   kendynet/core/src/log.c \
		   kendynet/core/asynnet/src/asynnet.c \
		   kendynet/core/asynnet/src/asynsock.c \
		   kendynet/core/asynnet/src/msgdisp.c \
		   kendynet/core/asynnet/src/asyncall.c \
		   kendynet/core/src/atomic_st.c \
		   kendynet/core/src/tls.c \
		   kendynet/core/src/lua_util.c\
		   kendynet/core/db/src/asynredis.c\
		   kendynet/core/db/src/asyndb.c\
		   kendynet/core/src/lua_util.c\
		   kendynet/core/src/kn_string.c\
		   kendynet/core/src/hash_map.c\
		   kendynet/core/src/minheap.c\
		   kendynet/core/src/lookup8.c\
		   kendynet/core/src/wpacket.c
		$(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
		ar -rc kendynet.a *.o
		rm -f *.o
game.a:\
		kendynet/game/src/astar.c\
		kendynet/game/src/aoi.c
		$(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
		ar -rc game.a *.o
		rm -f *.o	

testaoi:kendynet.a game.a $(TESTDIR)/testaoi.c $(TESTDIR)/testcommon.h
	$(CC) $(CFLAGS) -o testaoi $(TESTDIR)/testaoi.c kendynet.a game.a $(INCLUDE) $(LDFLAGS) $(DEFINE) 		
8puzzle:kendynet.a $(TESTDIR)/8puzzle.c $(TESTDIR)/testcommon.h
	$(CC) $(CFLAGS) -o 8puzzle $(TESTDIR)/8puzzle.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) 	
testmaze:kendynet.a $(TESTDIR)/testmaze.c $(TESTDIR)/testcommon.h
	$(CC) $(CFLAGS) -o testmaze $(TESTDIR)/testmaze.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) 
luanet:luanet.c kendynet.a
	$(CC) $(CFLAGS) -c $(SHARED) luanet.c $(INCLUDE) $(DEFINE) 
	$(CC) $(SHARED) -o luanet.so luanet.o kendynet.a $(LDFLAGS) $(DEFINE)
	rm -f *.o
testredis:kendynet.a $(TESTDIR)/testredis.c $(TESTDIR)/testcommon.h
	$(CC) $(CFLAGS) -o testredis $(TESTDIR)/testredis.c kendynet.a kendynet/deps/hiredis/libhiredis.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
packet:kendynet.a $(TESTDIR)/testpacket.c
	$(CC) $(CFLAGS) -o packet $(TESTDIR)/testpacket.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)	
gateservice:kendynet.a $(TESTDIR)/gateservice.c $(TESTDIR)/testcommon.h
	$(CC) $(CFLAGS) -o gateservice $(TESTDIR)/gateservice.c kendynet.a kendynet/deps/hiredis/libhiredis.a $(INCLUDE) $(LDFLAGS) $(DEFINE)	
asynserver:kendynet.a $(TESTDIR)/asynserver.c $(TESTDIR)/testcommon.h
	$(CC) $(CFLAGS) -o asynserver $(TESTDIR)/asynserver.c kendynet.a kendynet/deps/hiredis/libhiredis.a $(INCLUDE) $(LDFLAGS) $(DEFINE)	
tcpserver:kendynet.a $(TESTDIR)/benchserver.c $(TESTDIR)/testcommon.h
	$(CC) $(CFLAGS) -o tcpserver $(TESTDIR)/benchserver.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
server:kendynet.a $(TESTDIR)/server.c
	$(CC) $(CFLAGS) -o server $(TESTDIR)/server.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)	
tcpclient:kendynet.a $(TESTDIR)/benchclient.c $(TESTDIR)/testcommon.h
	$(CC) $(CFLAGS) -o tcpclient $(TESTDIR)/benchclient.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
timer:kendynet.a $(TESTDIR)/testtimer.c
	$(CC) $(CFLAGS) -o timer $(TESTDIR)/testtimer.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)				
msgque:kendynet.a $(TESTDIR)/testmq.c
	$(CC) $(CFLAGS) -o msgque $(TESTDIR)/testmq.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
systick:kendynet.a $(TESTDIR)/testgetsystick.c
	$(CC) $(CFLAGS) -o systick $(TESTDIR)/testgetsystick.c kendynet.a kendynet/deps/hiredis/libhiredis.a $(INCLUDE) $(LDFLAGS)	$(DEFINE)
atomicst:kendynet.a $(TESTDIR)/test_atomic_st.c
	$(CC) $(CFLAGS) -o atomicst $(TESTDIR)/test_atomic_st.c kendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE)
testexcp:kendynet.a $(TESTDIR)/testexception.c
	$(CC) $(CFLAGS) -o testexcp $(TESTDIR)/testexception.c kendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl
testlua:kendynet.a 	$(TESTDIR)/test.c
	$(CC) $(CFLAGS) -o testlua $(TESTDIR)/test.c kendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl -llua -lm
testasyncall:kendynet.a $(TESTDIR)/testasyncall.c $(TESTDIR)/testcommon.h
	$(CC) $(CFLAGS) -o testasyncall $(TESTDIR)/testasyncall.c kendynet.a kendynet/deps/hiredis/libhiredis.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
testlog:kendynet.a $(TESTDIR)/testlog.c $(TESTDIR)/testcommon.h
	$(CC) $(CFLAGS) -o testlog $(TESTDIR)/testlog.c kendynet.a kendynet/deps/hiredis/libhiredis.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)	

	
	
