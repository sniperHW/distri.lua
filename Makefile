CFLAGS = -g -Wall 
LDFLAGS = -lpthread -lrt -ltcmalloc
SHARED = -fPIC --shared
CC = gcc
INCLUDE = -I../cproactor/include -I.. -I/usr/local/include/luajit-2.0
DEFINE = -D_DEBUG -D_LINUX

kendynet.a: \
		   ../cproactor/src/kn_connector.c \
		   ../cproactor/src/kn_epoll.c \
		   ../cproactor/src/kn_except.c \
		   ../cproactor/src/kn_proactor.c \
		   ../cproactor/src/kn_ref.c \
		   ../cproactor/src/kn_acceptor.c \
		   ../cproactor/src/kn_socket.c \
		   ../cproactor/src/kn_datasocket.c \
		   ../cproactor/src/kendynet.c \
		   ../cproactor/src/kn_time.c \
		   ../cproactor/src/kn_thread.c
		   $(CC) $(CFLAGS) -c $^ $(INCLUDE) $(DEFINE)
	ar -rc kendynet.a *.o
	rm -f *.o
		
luanet:luanet.c lua_util.c kendynet.a
	$(CC) $(CFLAGS) -o luanet luanet.c lua_util.c kendynet.a /usr/local/lib/libluajit-5.1.a $(INCLUDE) $(LDFLAGS)	$(DEFINE) -rdynamic -ldl -lm

	
	
