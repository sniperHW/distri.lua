CFLAGS = -g -Wall -fno-strict-aliasing -rdynamic 
LDFLAGS = -lpthread -lrt -lm -ldl -ltcmalloc 
SHARED = -fPIC -shared
CC = gcc
INCLUDE = -I./include -I./src -I./
DEFINE = -D_DEBUG -D_LINUX 

kendynet.a: \
		   ./src/kn_epoll.c \
		   ./src/kn_timerfd.c \
		   ./src/kn_timer.c \
		   ./src/kn_time.c \
		   ./src/redisconn.c\
		   ./src/kn_socket.c
		   $(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
	ar -rc kendynet.a *.o
	rm -f *.o

testserver:testserver.c kendynet.a
	$(CC) $(CFLAGS) -o testserver testserver.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
testclient:testclient.c kendynet.a
	$(CC) $(CFLAGS) -o testclient testclient.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testredis:testredis.c kendynet.a						
	$(CC) $(CFLAGS) -o testredis testredis.c kendynet.a hiredis/libhiredis.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
testtimer:kendynet.a testtimer.c
	$(CC) $(CFLAGS) -o testtimer testtimer.c kendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl			
teststat:kendynet.a teststat.c
	$(CC) $(CFLAGS) -o teststat teststat.c kendynet.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl			
