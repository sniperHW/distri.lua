CFLAGS = -g -Wall 
LDFLAGS = -lpthread -lrt -ltcmalloc
SHARED = -fPIC --shared
CC = gcc
INCLUDE = -I../KendyNet/kendynet/core -I../KendyNet/kendynet -I.. -I/usr/local/include/luajit-2.0
DEFINE = -D_DEBUG -D_LINUX

kendynet.a: \
		   ../KendyNet/kendynet/core/src/buffer.c \
		   ../KendyNet/kendynet/core/src/connection.c \
		   ../KendyNet/kendynet/core/src/poller.c \
		   ../KendyNet/kendynet/core/src/epoll.c \
		   ../KendyNet/kendynet/core/src/except.c \
		   ../KendyNet/kendynet/core/src/kendynet.c \
		   ../KendyNet/kendynet/core/src/msgque.c \
		   ../KendyNet/kendynet/core/src/netservice.c \
		   ../KendyNet/kendynet/core/src/rbtree.c \
		   ../KendyNet/kendynet/core/src/rpacket.c \
		   ../KendyNet/kendynet/core/src/socket.c \
		   ../KendyNet/kendynet/core/src/sock_util.c \
		   ../KendyNet/kendynet/core/src/spinlock.c \
		   ../KendyNet/kendynet/core/src/systime.c \
		   ../KendyNet/kendynet/core/src/thread.c \
		   ../KendyNet/kendynet/core/src/timer.c \
		   ../KendyNet/kendynet/core/src/uthread.c \
		   ../KendyNet/kendynet/core/src/refbase.c \
		   ../KendyNet/kendynet/core/src/log.c \
		   ../KendyNet/kendynet/core/asynnet/src/asynnet.c \
		   ../KendyNet/kendynet/core/asynnet/src/asynsock.c \
		   ../KendyNet/kendynet/core/asynnet/src/msgdisp.c \
		   ../KendyNet/kendynet/core/asynnet/src/asyncall.c \
		   ../KendyNet/kendynet/core/src/atomic_st.c \
		   ../KendyNet/kendynet/core/src/tls.c \
		   ../KendyNet/kendynet/core/src/lua_util.c\
		   ../KendyNet/kendynet/core/src/lua_util.c\
		   ../KendyNet/kendynet/core/src/kn_string.c\
		   ../KendyNet/kendynet/core/src/hash_map.c\
		   ../KendyNet/kendynet/core/src/minheap.c\
		   ../KendyNet/kendynet/core/src/lookup8.c\
		   ../KendyNet/kendynet/core/src/wpacket.c
		$(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
		ar -rc kendynet.a *.o
		rm -f *.o
luanet:luanet.c kendynet.a
	$(CC) $(CFLAGS) -c $(SHARED) luanet.c $(INCLUDE) $(DEFINE) 
	$(CC) $(SHARED) -o luanet.so luanet.o kendynet.a $(LDFLAGS) $(DEFINE)
	rm -f *.o

	
	
