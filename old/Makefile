CFLAGS = -g -Wall -fno-strict-aliasing -rdynamic 
LDFLAGS = -lpthread -lrt -llua5.2 -lm -ldl -ltcmalloc 
SHARED = -fPIC -shared
CC = gcc
INCLUDE = -IKendyNet/include -I.. -I../deps -I/usr/include/lua5.2
DEFINE = -D_DEBUG -D_LINUX 

kendynet.a: \
		   KendyNet/src/kn_connector.c \
		   KendyNet/src/kn_epoll.c \
		   KendyNet/src/kn_except.c \
		   KendyNet/src/kn_proactor.c \
		   KendyNet/src/kn_ref.c \
		   KendyNet/src/kn_acceptor.c \
		   KendyNet/src/kn_fd.c \
		   KendyNet/src/kn_datasocket.c \
		   KendyNet/src/kendynet.c \
		   KendyNet/src/kn_time.c \
		   KendyNet/src/kn_timer.c\
		   KendyNet/src/kn_thread.c\
		   KendyNet/src/spinlock.c\
		   KendyNet/src/lua_util.c\
		   KendyNet/src/kn_timerfd.c\
		   KendyNet/src/kn_channel.c
		   $(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
	ar -rc kendynet.a *.o
	rm -f *.o

cjson.so:
	cp ../deps/lua-cjson-2.1.0/cjson.so .
		
distri:src/distri.c kendynet.a cjson.so
	$(CC) $(CFLAGS) -o distri src/distri.c kendynet.a  $(INCLUDE) $(LDFLAGS) $(DEFINE) 

	
	
