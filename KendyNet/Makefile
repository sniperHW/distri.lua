PLAT= none
PLATS= bsd linux linux-sample bsd-sample
CFLAGS = -g -Wall -fno-strict-aliasing
LDFLAGS = -lpthread -lrt -lm -lssl -lcrypto
SHARED   =   -fPIC
DEPDIR = ../deps
INCLUDE = -I./include -I./src -I./ -I$(DEPDIR) -I$(DEPDIR)/lua-5.2.3/src -I$(DEPDIR)/curl-7.38.0/include

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
	   src/stream_conn.c \
	   src/kn_thread.c \
	   src/kn_thread_mailbox.c \
	   src/hash_map.c \
	   src/kn_except.c \
	   src/lookup8.c \
	   src/spinlock.c \
	   src/log.c \
	   src/kn_string.c \
	   src/minheap.c \
	   src/tls.c \
	   src/rbtree.c \
	   src/kn_msgque.c \
	   src/kn_daemonize.c \
	   src/lua_util.c \
	   src/kn_curl.c\
	   src/uthread.c\
	   src/kn_objpool.c\
	   src/kn_chr_dev.c\
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
		 sample/testmsgque.c\
		 sample/daemonserver.c\
		 sample/testcurl.c\
		 sample/testlfstack.c\
		 sample/testminheap.c\
		 sample/ccoroutine.c\
		 sample/objpool.c\
		 sample/testchrdev.c\
		 sample/testatomicst.c\
		 sample/ssl_server.c\
		 sample/ssl_client.c

	   
linux:$(source)
	$(CC) $(SHARED) $(CFLAGS) -c $(source) $(INCLUDE) $(DEFINE) -D_LINUX
	ar -rc libkendynet.a *.o

linux-sample:linux $(sample-source)
	$(CC) $(CFLAGS) -c $(sample-source) $(INCLUDE) $(DEFINE) -D_LINUX
	$(CC) $(CFLAGS) -o broadcast_svr broadcast_svr.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o broadcast_cli broadcast_cli.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o tcpserver tcpserver.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o pingpong pingpong.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testserver testserver.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testclient testclient.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testredis testredis.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a $(DEPDIR)/hiredis/libhiredis.a $(INCLUDE) $(LDFLAGS) $(DEFINE)   	 			
	$(CC) $(CFLAGS) -o testtimer testtimer.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testmailbox testmailbox.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testlog testlog.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testlua testlua.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(DEPDIR)/lua-5.2.3/src/liblua.a -ldl $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testmsgque testmsgque.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o daemonserver daemonserver.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testcurl testcurl.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE) -lcurl
	$(CC) $(CFLAGS) -o testlfstack testlfstack.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testminheap testminheap.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o ccoroutine ccoroutine.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o objpool objpool.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testchrdev testchrdev.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o testatomicst testatomicst.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o ssl_server ssl_server.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)
	$(CC) $(CFLAGS) -o ssl_client ssl_client.o libkendynet.a $(DEPDIR)/jemalloc/lib/libjemalloc.a  $(INCLUDE) $(LDFLAGS) $(DEFINE)

bsd-sample:bsd $(sample-source)
	$(CC) $(CFLAGS) -c $(sample-source) $(INCLUDE) $(DEFINE) -D_BSD
	$(CC) $(CFLAGS) -o broadcast_svr broadcast_svr.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o broadcast_cli broadcast_cli.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o tcpserver tcpserver.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o pingpong pingpong.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o testserver testserver.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o testclient testclient.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o testredis testredis.o libkendynet.a $(DEPDIR)/hiredis/libhiredis.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o testtimer testtimer.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o testmailbox testmailbox.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o testlog testlog.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o testlua testlua.o libkendynet.a $(DEPDIR)/lua-5.2.3/src/liblua.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o testmsgque testmsgque.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o daemonserver daemonserver.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o testcurl testcurl.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lcurl -lexecinfo
	$(CC) $(CFLAGS) -o testlfstack testlfstack.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o testminheap testminheap.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o ccoroutine ccoroutine.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o objpool objpool.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o testchrdev testchrdev.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o testatomicst testatomicst.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o ssl_server ssl_server.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo
	$(CC) $(CFLAGS) -o ssl_client ssl_client.o libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lexecinfo


bsd:$(source)
	$(CC) $(SHARED) $(CFLAGS) -c $(source) $(INCLUDE) $(DEFINE) -D_BSD
	ar -rc libkendynet.a *.o	

none:
	@echo "Please do 'make PLATFORM' where PLATFORM is one of these:"
	@echo "   $(PLATS)"
