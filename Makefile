CFLAGS = -g -Wall -fno-strict-aliasing -rdynamic 
LDFLAGS = -lpthread -lrt -llua5.2 -lm -ldl -ltcmalloc 
SHARED = -fPIC -shared
CC = gcc
DEFINE = -D_DEBUG -D_LINUX 
INCLUDE = -I../KendyNet/include -I./deps -I/usr/include/lua5.2 -I../deps
		
test:test.c
	$(CC) $(CFLAGS) -o test test.c $(LDFLAGS) $(DEFINE)
	
testusrdata:testusrdata.c
	$(CC) $(CFLAGS) -o testusrdata testusrdata.c $(LDFLAGS) $(DEFINE) 
	

kendynet.a: \
		   ../KendyNet/src/kn_epoll.c \
		   ../KendyNet/src/kn_timerfd.c \
		   ../KendyNet/src/kn_timer.c \
		   ../KendyNet/src/kn_time.c \
		   ../KendyNet/src/redisconn.c\
		   ../KendyNet/src/kn_chr_dev.c\
		   ../KendyNet/src/kn_refobj.c \
		   ../KendyNet/src/rpacket.c \
		   ../KendyNet/src/wpacket.c \
		   ../KendyNet/src/packet.c \
		   ../KendyNet/src/kn_socket.c \
		   ../KendyNet/src/kn_refobj.c \
		   ../KendyNet/src/stream_conn.c \
		   ../KendyNet/src/kn_thread.c \
		   ../KendyNet/src/kn_thread_mailbox.c \
		   ../KendyNet/src/hash_map.c \
		   ../KendyNet/src/kn_except.c \
		   ../KendyNet/src/lookup8.c \
		   ../KendyNet/src/spinlock.c \
		   ../KendyNet/src/log.c \
		   ../KendyNet/src/kn_string.c \
		   ../KendyNet/src/minheap.c \
		   ../KendyNet/src/tls.c \
		   ../KendyNet/src/rbtree.c \
		   ../KendyNet/src/buffer.c
		   $(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
	ar -rc kendynet.a *.o
	rm -f *.o	 

distrilua:kendynet.a\
		  event_version/src/luasocket.h\
		  event_version/src/luasocket.c\
		  event_version/src/distri.c	
		$(CC) $(CFLAGS) -o distrilua event_version/src/distri.c event_version/src/luasocket.c kendynet.a ./deps/hiredis/libhiredis.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
		
distrilua2:kendynet.a\
		   coroutine_version/src/luasocket.h\
		   coroutine_version/src/luasocket.c\
		   coroutine_version/src/distri.c	
		$(CC) $(CFLAGS) -o distrilua2 coroutine_version/src/distri.c coroutine_version/src/luasocket.c kendynet.a ./deps/hiredis/libhiredis.a $(INCLUDE) $(LDFLAGS) $(DEFINE)		
	    mv distrilua2 coroutine_version
