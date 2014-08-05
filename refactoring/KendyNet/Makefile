CFLAGS = -g -Wall -fno-strict-aliasing -rdynamic 
LDFLAGS = -lpthread -lrt -lm -ldl -ltcmalloc 
SHARED = -fPIC -shared
CC = gcc
INCLUDE = -I./include -I./src -I./ -I../../deps
DEFINE = -D_DEBUG -D_LINUX 

kendynet.a: \
		   ./src/kn_epoll.c \
		   ./src/kn_timerfd.c \
		   ./src/kn_timer.c \
		   ./src/kn_time.c \
		   ./src/redisconn.c\
		   ./src/kn_chr_dev.c\
		   ./src/kn_refobj.c \
		   ./src/rpacket.c \
		   ./src/wpacket.c \
		   ./src/packet.c \
		   ./src/kn_socket.c \
		   ./src/kn_refobj.c \
		   ./src/stream_conn.c \
		   ./src/kn_thread.c \
		   ./src/kn_thread_mailbox.c \
		   ./src/hash_map.c \
		   ./src/kn_except.c \
		   ./src/lookup8.c \
		   ./src/spinlock.c \
		   ./src/log.c \
		   ./src/kn_string.c \
		   ./src/minheap.c \
		   ./src/tls.c \
		   ./src/rbtree.c \
		   ./src/buffer.c
		   $(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
	ar -rc kendynet.a *.o
	rm -f *.o
	
broadcast_svr:broadcast_svr.c kendynet.a
	$(CC) $(CFLAGS) -o broadcast_svr broadcast_svr.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
broadcast_cli:broadcast_cli.c kendynet.a
	$(CC) $(CFLAGS) -o broadcast_cli broadcast_cli.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)		

tcpserver:tcpserver.c kendynet.a
	$(CC) $(CFLAGS) -o tcpserver tcpserver.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)

pingpong:pingpong.c kendynet.a
	$(CC) $(CFLAGS) -o pingpong pingpong.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testserver:testserver.c kendynet.a
	$(CC) $(CFLAGS) -o testserver testserver.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
testclient:testclient.c kendynet.a
	$(CC) $(CFLAGS) -o testclient testclient.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testredis:testredis.c kendynet.a						
	$(CC) $(CFLAGS) -o testredis testredis.c kendynet.a ../../deps/hiredis/libhiredis.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testtimer:kendynet.a testtimer.c
	$(CC) $(CFLAGS) -o testtimer testtimer.c kendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl			
teststat:kendynet.a teststat.c
	$(CC) $(CFLAGS) -o teststat teststat.c kendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl
testchrdev:kendynet.a testchrdev.c
	$(CC) $(CFLAGS) -o testchrdev testchrdev.c kendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl				
testmailbox:kendynet.a testmailbox.c
	$(CC) $(CFLAGS) -o testmailbox testmailbox.c kendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl
log:testlog.c kendynet.a						
	$(CC) $(CFLAGS) -o log testlog.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testlua:kendynet.a testlua.c	
	$(CC) $(CFLAGS) -o testlua testlua.c kendynet.a  $(INCLUDE) $(LDFLAGS) -llua5.2 -ldl -lm $(DEFINE) 		
					
