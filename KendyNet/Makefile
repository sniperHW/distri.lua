CFLAGS = -g -Wall -fno-strict-aliasing -rdynamic 
LDFLAGS = -lpthread -lrt -llua -lm -ldl -ltcmalloc 
SHARED = -fPIC -shared
CC = gcc
INCLUDE = -I./include -I./src
DEFINE = -D_DEBUG -D_LINUX 

kendynet.a: \
		   ./src/kn_epoll.c \
		   ./src/kn_timerfd.c \
		   ./src/kn_timer.c \
		   ./src/kn_time.c \
		   ./src/kn_socket.c
		   $(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
	ar -rc kendynet.a *.o
	rm -f *.o

testserver:testserver.c kendynet.a
	$(CC) $(CFLAGS) -o testserver testserver.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
	
testclient:testclient.c kendynet.a
	$(CC) $(CFLAGS) -o testclient testclient.c kendynet.a $(INCLUDE) $(LDFLAGS) $(DEFINE)	
