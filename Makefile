CFLAGS = -g -Wall -fno-strict-aliasing -rdynamic 
LDFLAGS = -lpthread -lrt -llua5.2 -lm -ldl -ljemalloc
SHARED = -fPIC -shared
CC = gcc
DEFINE = -D_DEBUG -D_LINUX 
INCLUDE = -Ideps/KendyNet/include -Ideps -I/usr/include/lua5.2 
LIB = -Ldeps/jemalloc/lib
		
test:test.c
	$(CC) $(CFLAGS) -o test test.c $(LDFLAGS) $(DEFINE)
	
testusrdata:testusrdata.c
	$(CC) $(CFLAGS) -o testusrdata testusrdata.c $(LDFLAGS) $(DEFINE) 
	

kendynet.a: \
		   deps/KendyNet/src/kn_epoll.c \
		   deps/KendyNet/src/kn_timerfd.c \
		   deps/KendyNet/src/kn_timer.c \
		   deps/KendyNet/src/kn_time.c \
		   deps/KendyNet/src/redisconn.c\
		   deps/KendyNet/src/kn_chr_dev.c\
		   deps/KendyNet/src/kn_refobj.c \
		   deps/KendyNet/src/rpacket.c \
		   deps/KendyNet/src/wpacket.c \
		   deps/KendyNet/src/packet.c \
		   deps/KendyNet/src/kn_socket.c \
		   deps/KendyNet/src/kn_refobj.c \
		   deps/KendyNet/src/stream_conn.c \
		   deps/KendyNet/src/kn_thread.c \
		   deps/KendyNet/src/kn_thread_mailbox.c \
		   deps/KendyNet/src/hash_map.c \
		   deps/KendyNet/src/kn_except.c \
		   deps/KendyNet/src/lookup8.c \
		   deps/KendyNet/src/spinlock.c \
		   deps/KendyNet/src/log.c \
		   deps/KendyNet/src/kn_string.c \
		   deps/KendyNet/src/minheap.c \
		   deps/KendyNet/src/tls.c \
		   deps/KendyNet/src/rbtree.c \
		   deps/KendyNet/src/lua_util.c \
		   deps/KendyNet/src/buffer.c
		   $(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
	ar -rc kendynet.a *.o
	rm -f *.o	 

distrilua:kendynet.a\
		  src/luasocket.h\
		  src/luasocket.c\
		  src/luabytebuffer.h\
		  src/luabytebuffer.c\
		  src/distri.c	
		$(CC) $(CFLAGS) -o distrilua src/distri.c src/luasocket.c src/luabytebuffer.c kendynet.a deps/hiredis/libhiredis.a $(LIB) $(INCLUDE) $(LDFLAGS) $(DEFINE)
