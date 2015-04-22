#PLAT= none
#PLATS= bsd linux linux-sample bsd-sample
CFLAGS = -g -Wall -fno-strict-aliasing
LDFLAGS = -lpthread -lrt -lm -lssl -lcrypto
SHARED   =   -fPIC
DEPDIR = ../deps
INCLUDE = -I./include -I./src -I./ -I$(DEPDIR) -I$(DEPDIR)/lua-5.3.0/src -I$(DEPDIR)/curl-7.38.0/include
DEFINE =
LIBNAME = libkendynet.a

# Platform-specific overrides
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(uname_S),Linux)
	LDFLAGS += -ldl
	DEFINE += -D_LINUX
endif

ifeq ($(uname_S),FreeBSD)
	LDFLAGS += -lexecinfo
	DEFINE += -D_BSD
endif

source =  src/kn_engine.c \
	   src/kn_timerfd.c \
	   src/kn_timer.c \
	   src/kn_time.c \
	   src/redisconn.c\
	   src/kn_refobj.c \
	   src/rpacket.c \
	   src/wpacket.c \
	   src/packet.c \
	   src/kn_socket.c \
	   src/kn_refobj.c \
	   src/connection.c \
	   src/kn_thread.c \
	   src/kn_thread_mailbox.c \
	   src/kn_except.c \
	   src/lookup8.c \
	   src/spinlock.c \
	   src/log.c \
	   src/kn_string.c \
	   src/minheap.c \
	   src/tls.c \
	   src/rbtree.c \
	   src/kn_daemonize.c \
	   src/lua_util.c \
	   src/kn_curl.c\
	   src/kn_objpool.c\
	   src/kn_chr_dev.c\
	   src/kendynet.c\
	   src/kn_stream_socket.c\
	   src/kn_datagram_socket.c\
	   src/datagram.c\
	   src/lua_util_packet.c\
	   src/buffer.c

sample-source = sample/broadcast_svr.c\
		 sample/broadcast_cli.c\
		 sample/tcpserver.c\
		 sample/pingpong.c\
		 sample/testserver.c\
		 sample/testclient.c\
		 sample/testredis.c\
		 sample/testtimer.c\
		 sample/testmailbox.c\
		 sample/testlog.c\
		 sample/testlua.c\
		 sample/daemonserver.c\
		 sample/testcurl.c\
		 sample/testlfstack.c\
		 sample/testminheap.c\
		 sample/objpool.c\
		 sample/testchrdev.c\
		 sample/testatomicst.c\
		 sample/ssl_server.c\
		 sample/ssl_client.c\
		 sample/testpacket.c\
		 sample/udpserver.c\
		 sample/udpclient.c\
		 sample/testrefobj.c\
		 sample/testexception.c
	   
release:$(source)
	$(CC) $(SHARED) -O2 $(CFLAGS) -c $(source) $(INCLUDE) $(DEFINE)
	ar -rc $(LIBNAME) *.o

debug:$(source)
	$(CC) $(SHARED) $(CFLAGS) -c $(source) $(INCLUDE) $(DEFINE) -D_DEBUG
	ar -rc $(LIBNAME) *.o

sample:$(LIBNAME) $(sample-source)
	$(CC) $(CFLAGS) -c  $(sample-source) $(INCLUDE) $(DEFINE)
	$(CC) $(CFLAGS) -o broadcast_svr broadcast_svr.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o broadcast_cli broadcast_cli.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o tcpserver tcpserver.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o pingpong pingpong.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testserver testserver.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testclient testclient.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testredis testredis.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a $(DEPDIR)/hiredis/libhiredis.a $(INCLUDE) $(LDFLAGS) $(DEFINE)   	 			
	$(CC) $(CFLAGS) -o testtimer testtimer.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testmailbox testmailbox.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testlog testlog.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testlua testlua.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(DEPDIR)/lua-5.3.0/src/liblua.a -ldl $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o daemonserver daemonserver.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testcurl testcurl.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE) -lcurl
	$(CC) $(CFLAGS) -o testlfstack testlfstack.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testminheap testminheap.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o objpool objpool.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testchrdev testchrdev.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testatomicst testatomicst.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o ssl_server ssl_server.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o ssl_client ssl_client.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testpacket testpacket.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o udpserver udpserver.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o udpclient udpclient.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testrefobj testrefobj.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testexception testexception.o $(LIBNAME) $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	

clean:
	rm *.o
	rm *.a