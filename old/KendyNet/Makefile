CFLAGS = -g -Wall -fno-strict-aliasing -std=gnu99
LDFLAGS = -lpthread -lrt -lm -ltcmalloc
SHARED = -fPIC --shared
CC = gcc
INCLUDE =  -I./include -I./

kendynet.a: \
		   src/kn_connector.c \
		   src/kn_epoll.c \
		   src/kn_except.c \
		   src/kn_proactor.c \
		   src/kn_ref.c \
		   src/kn_acceptor.c \
		   src/kn_fd.c \
		   src/kn_datasocket.c \
		   src/kendynet.c \
		   src/kn_time.c \
		   src/kn_thread.c\
		   src/buffer.c\
		   src/kn_string.c\
		   src/wpacket.c\
		   src/rpacket.c\
		   src/kn_timer.c\
		   src/kn_stream_conn.c\
		   src/kn_stream_conn_server.c\
		   src/kn_stream_conn_client.c\
		   src/minheap.c\
		   src/hash_map.c\
		   src/rbtree.c\
		   src/spinlock.c\
		   src/obj_allocator.c\
		   src/log.c\
		   src/redisconn.c\
		   src/tls.c\
		   src/lua_util.c\
		   src/lookup8.c\
		   src/kn_timerfd.c\
		   src/kn_channel.c
		$(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
		ar -rc kendynet.a *.o
		rm -f *.o

stream_server:stream_server.c kendynet.a
	$(CC) $(CFLAGS) -o stream_server stream_server.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)

stream_client:stream_client.c kendynet.a
	$(CC) $(CFLAGS) -o stream_client stream_client.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
		
tcp_echoserver:tcp_echoserver.c kendynet.a
	$(CC) $(CFLAGS) -o tcpechosever tcp_echoserver.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
tcp_echoclient:tcp_echoclient.c kendynet.a
	$(CC) $(CFLAGS) -o tcpechoclient tcp_echoclient.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
unix_echoserver:unix_echoserver.c kendynet.a
	$(CC) $(CFLAGS) -o unixechosever unix_echoserver.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
unix_echoclient:unix_echoclient.c kendynet.a
	$(CC) $(CFLAGS) -o unixechoclient unix_echoclient.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
udp_echoserver:udp_echoserver.c kendynet.a
	$(CC) $(CFLAGS) -o udpechosever udp_echoserver.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
udp_echoclient:udp_echoclient.c kendynet.a
	$(CC) $(CFLAGS) -o udpechoclient udp_echoclient.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
udpunix_echoserver:udpunix_echoserver.c kendynet.a
	$(CC) $(CFLAGS) -o udpunixechosever udpunix_echoserver.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
udpunix_echoclient:udpunix_echoclient.c kendynet.a
	$(CC) $(CFLAGS) -o udpunixechoclient udpunix_echoclient.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
ringque:testmq.c kendynet.a						
	$(CC) $(CFLAGS) -o ringque testmq.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
channel:channel.c kendynet.a						
	$(CC) $(CFLAGS) -o channel channel.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)

log:testlog.c kendynet.a						
	$(CC) $(CFLAGS) -o log testlog.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
testredis:testredis.c kendynet.a						
	$(CC) $(CFLAGS) -o testredis testredis.c kendynet.a hiredis/libhiredis.a $(INCLUDE) $(LDFLAGS) $(DEFINE)	

testheap:testminheap.c kendynet.a						
	$(CC) $(CFLAGS) -o testheap testminheap.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)	
	
testexcp:kendynet.a testexception.c
	$(CC) $(CFLAGS) -o testexcp testexception.c kendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl
	
testtimer:kendynet.a testtimer.c
	$(CC) $(CFLAGS) -o testtimer testtimer.c kendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl	

	
testlua:kendynet.a testlua.c	
	$(CC) $(CFLAGS) -o testlua testlua.c kendynet.a  $(INCLUDE) $(LDFLAGS) -llua -ldl -lm $(DEFINE) 	
testhash:kendynet.a testhash.c	
	$(CC) $(CFLAGS) -o testhash testhash.c kendynet.a  $(INCLUDE) $(LDFLAGS) -ldl -lm $(DEFINE) 	
testspin:kendynet.a testspin.c	
	$(CC) $(CFLAGS) -o testspin testspin.c kendynet.a  $(INCLUDE) $(LDFLAGS) -ldl -lm $(DEFINE) 	
testatomicst:kendynet.a testatomicst.c	
	$(CC) $(CFLAGS) -o testatomicst testatomicst.c kendynet.a  $(INCLUDE) $(LDFLAGS) -ldl -lm $(DEFINE) 	
