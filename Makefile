CFLAGS = -O2 -g -Wall -fno-strict-aliasing -rdynamic 
LDFLAGS = -lpthread -lrt -llua -lm -ldl -ltcmalloc 
SHARED = -fPIC -shared
CC = gcc
INCLUDE = -I./deps/cproactor/include -I..
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
		   ./deps/cproactor/src/kn_thread.c
		   $(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
	ar -rc kendynet.a *.o
	rm -f *.o

cjson.so:
	cp deps/lua-cjson-2.1.0/cjson.so .
		
luanet:src/luanet.c src/lua_util.c kendynet.a cjson.so
	$(CC) $(CFLAGS) -o luanet src/luanet.c src/lua_util.c kendynet.a  $(INCLUDE) $(LDFLAGS)	$(DEFINE) 

	
	
