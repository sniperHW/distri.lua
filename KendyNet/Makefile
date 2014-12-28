CFLAGS = -O2 -g -Wall -fno-strict-aliasing
LDFLAGS = -lpthread -lrt -lm
SHARED = -fPIC -shared
CC = gcc
DEPDIR = ../deps
INCLUDE = -I./include -I./src -I./ -I$(DEPDIR) -I$(DEPDIR)/lua-5.2.3/src 
DEFINE = -D_DEBUG -D_LINUX 

source =  src/kn_epoll.c \
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
	   src/buffer.c

libkendynet.a: $(source)
	$(CC) $(SHARED) $(CFLAGS) -c $(source) $(INCLUDE) $(DEFINE)
	ar -rc libkendynet.a *.o
	
broadcast_svr:broadcast_svr.c libkendynet.a
	$(CC) $(CFLAGS) -o broadcast_svr broadcast_svr.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
broadcast_cli:broadcast_cli.c libkendynet.a
	$(CC) $(CFLAGS) -o broadcast_cli broadcast_cli.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)		

tcpserver:tcpserver.c libkendynet.a
	$(CC) $(CFLAGS) -o tcpserver tcpserver.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)

pingpong:pingpong.c libkendynet.a
	$(CC) $(CFLAGS) -o pingpong pingpong.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testserver:testserver.c libkendynet.a
	$(CC) $(CFLAGS) -o testserver testserver.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
testclient:testclient.c libkendynet.a
	$(CC) $(CFLAGS) -o testclient testclient.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testredis:testredis.c libkendynet.a						
	$(CC) $(CFLAGS) -o testredis testredis.c libkendynet.a ../../deps/hiredis/libhiredis.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testtimer:libkendynet.a testtimer.c
	$(CC) $(CFLAGS) -o testtimer testtimer.c libkendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl							
testmailbox:libkendynet.a testmailbox.c
	$(CC) $(CFLAGS) -o testmailbox testmailbox.c libkendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl
log:testlog.c libkendynet.a						
	$(CC) $(CFLAGS) -o log testlog.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testlua:libkendynet.a testlua.c	
	$(CC) $(CFLAGS) -o testlua testlua.c libkendynet.a  $(INCLUDE) $(LDFLAGS) -llua -ldl -lm $(DEFINE) -L $(DEPDIR)/lua-5.2.3/src
	
testmsgque:libkendynet.a testmsgque.c
	$(CC) $(CFLAGS) -o testmsgque testmsgque.c libkendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl
	
mysql_proxy:libkendynet.a mysql_proxy.c
	$(CC) $(CFLAGS) -o mysql_proxy mysql_proxy.c libkendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl -lmysqlclient -I/usr/include/mysql	
	
daemonserver:daemonserver.c libkendynet.a
	$(CC) $(CFLAGS) -o daemonserver daemonserver.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)

testhttp:testhttp.c libkendynet.a
	$(CC) $(CFLAGS) -o testhttp testhttp.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
testcurl:testcurl.c libkendynet.a
	$(CC) $(CFLAGS) -o testcurl testcurl.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE) -lcurl	

testlfstack:libkendynet.a testlfstack.c
	$(CC) $(CFLAGS) -o testlfstack testlfstack.c libkendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)						
					
