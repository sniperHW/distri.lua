CFLAGS = -g -Wall -fno-strict-aliasing -rdynamic 
LDFLAGS = -lpthread -lrt -llua -lm -ldl -ltcmalloc 
SHARED = -fPIC -shared
CC = gcc
INCLUDE = -I./deps/cproactor/include -I.. -I./deps
DEFINE = -D_DEBUG -D_LINUX 

kendynet.a: \
		   ./deps/cproactor/src/kn_connector.c \
		   ./deps/cproactor/src/kn_epoll.c \
		   ./deps/cproactor/src/kn_except.c \
		   ./deps/cproactor/src/kn_proactor.c \
		   ./deps/cproactor/src/kn_ref.c \
		   ./deps/cproactor/src/kn_acceptor.c \
		   ./deps/cproactor/src/kn_fd.c \
		   ./deps/cproactor/src/kn_datasocket.c \
		   ./deps/cproactor/src/kendynet.c \
		   ./deps/cproactor/src/kn_time.c \
		   ./deps/cproactor/src/kn_thread.c\
		   ./deps/cproactor/src/spinlock.c\
		   ./deps/cproactor/src/lua_util.c\
		   ./deps/cproactor/src/kn_channel.c
		   $(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
	ar -rc kendynet.a *.o
	rm -f *.o

cjson.so:
	cp deps/lua-cjson-2.1.0/cjson.so .
		
distri:src/distri.c kendynet.a cjson.so
	$(CC) $(CFLAGS) -o distri src/distri.c kendynet.a  $(INCLUDE) $(LDFLAGS) $(DEFINE) 

	
	
