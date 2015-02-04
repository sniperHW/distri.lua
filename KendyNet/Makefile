CFLAGS = -g -Wall -fno-strict-aliasing
LDFLAGS = -lpthread -lrt -lm -lssl -lcrypto
SHARED = -fPIC
CC = cc
DEPDIR = ../deps
INCLUDE = -I./include -I./src -I./ -I$(DEPDIR) -I$(DEPDIR)/lua-5.2.3/src -I$(DEPDIR)/curl-7.38.0/include
DEFINE = -D_DEBUG -D_LINUX
LIB = -L deps/jemalloc/lib -L KendyNet -L deps/hiredis/ -L deps/lua-5.2.3/src 
SAMPLE = ./sample

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

libkendynet.a: $(source)
	$(CC) $(SHARED) $(CFLAGS) -c $(source) $(INCLUDE) $(DEFINE)
	ar -rc libkendynet.a *.o
	
broadcast_svr:$(SAMPLE)/broadcast_svr.c libkendynet.a
	$(CC) $(CFLAGS) -o broadcast_svr $(SAMPLE)/broadcast_svr.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
broadcast_cli:$(SAMPLE)/broadcast_cli.c libkendynet.a
	$(CC) $(CFLAGS) -o broadcast_cli $(SAMPLE)/broadcast_cli.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)		

tcpserver:$(SAMPLE)/tcpserver.c libkendynet.a
	$(CC) $(CFLAGS) -o tcpserver $(SAMPLE)/tcpserver.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)

pingpong:$(SAMPLE)/pingpong.c libkendynet.a
	$(CC) $(CFLAGS) -o pingpong $(SAMPLE)/pingpong.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testserver:$(SAMPLE)/testserver.c libkendynet.a
	$(CC) $(CFLAGS) -o testserver $(SAMPLE)/testserver.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
testclient:$(SAMPLE)/testclient.c libkendynet.a
	$(CC) $(CFLAGS) -o testclient $(SAMPLE)/testclient.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testredis:$(SAMPLE)/testredis.c libkendynet.a						
	$(CC) $(CFLAGS) -o testredis $(SAMPLE)/testredis.c libkendynet.a ../../deps/hiredis/libhiredis.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testtimer:libkendynet.a $(SAMPLE)/testtimer.c
	$(CC) $(CFLAGS) -o testtimer $(SAMPLE)/testtimer.c libkendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE)  -ldl							
testmailbox:libkendynet.a $(SAMPLE)/testmailbox.c
	$(CC) $(CFLAGS) -o testmailbox $(SAMPLE)/testmailbox.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)  -ldl
log:$(SAMPLE)/testlog.c libkendynet.a						
	$(CC) $(CFLAGS) -o log $(SAMPLE)/testlog.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testlua:libkendynet.a $(SAMPLE)/testlua.c	
	$(CC) $(CFLAGS) -o testlua $(SAMPLE)/testlua.c libkendynet.a  $(INCLUDE) $(LDFLAGS) $(DEPDIR)/lua-5.2.3/src/liblua.a -lm -ldl $(DEFINE)
	
testmsgque:libkendynet.a $(SAMPLE)/testmsgque.c
	$(CC) $(CFLAGS) -o testmsgque $(SAMPLE)/testmsgque.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)  -ldl
	
mysql_proxy:libkendynet.a $(SAMPLE)/mysql_proxy.c
	$(CC) $(CFLAGS) -o mysql_proxy $(SAMPLE)/mysql_proxy.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)  -ldl -lmysqlclient -I/usr/include/mysql	
	
daemonserver:$(SAMPLE)/daemonserver.c libkendynet.a
	$(CC) $(CFLAGS) -o daemonserver $(SAMPLE)/daemonserver.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)

testhttp:$(SAMPLE)/testhttp.c libkendynet.a
	$(CC) $(CFLAGS) -o testhttp $(SAMPLE)/testhttp.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
testcurl:$(SAMPLE)/testcurl.c libkendynet.a
	$(CC) $(CFLAGS) -o testcurl $(SAMPLE)/testcurl.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lcurl	

testlfstack:libkendynet.a $(SAMPLE)/testlfstack.c
	$(CC) $(CFLAGS) -o testlfstack $(SAMPLE)/testlfstack.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)	

testminheap:libkendynet.a $(SAMPLE)/testminheap.c
	$(CC) $(CFLAGS) -o testminheap $(SAMPLE)/testminheap.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)

ccoroutine:libkendynet.a $(SAMPLE)/ccoroutine.c
	$(CC) $(CFLAGS) -o ccoroutine $(SAMPLE)/ccoroutine.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)	

objpool:libkendynet.a $(SAMPLE)/objpool.c
	$(CC) $(CFLAGS) -o objpool $(SAMPLE)/objpool.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testchrdev:libkendynet.a $(SAMPLE)/testchrdev.c
	$(CC) $(CFLAGS) -o testchrdev $(SAMPLE)/testchrdev.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testatomicst:libkendynet.a $(SAMPLE)/testatomicst.c include/lockfree/atomic_st.h
	$(CC) $(CFLAGS) -o testatomicst $(SAMPLE)/testatomicst.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)

ssl_server:$(SAMPLE)/ssl_server.c libkendynet.a
	$(CC) $(CFLAGS) -o ssl_server $(SAMPLE)/ssl_server.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)	

ssl_client:$(SAMPLE)/ssl_client.c libkendynet.a
	$(CC) $(CFLAGS) -o ssl_client $(SAMPLE)/ssl_client.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)	

testlist:$(SAMPLE)/testlist.c libkendynet.a
	$(CC) $(CFLAGS) -o testlist $(SAMPLE)/testlist.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)	

quicksort:$(SAMPLE)/quicksort.c libkendynet.a
	$(CC) $(CFLAGS) -o quicksort $(SAMPLE)/quicksort.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)												
					
