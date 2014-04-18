CFLAGS = -g -Wall -fno-strict-aliasing
LDFLAGS = -lpthread -lrt -llua -lm -ldl -ltcmalloc 
SHARED = -fPIC --shared
CC = gcc
INCLUDE = -I../cproactor/include -I..
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
		
luanet:src/luanet.c src/lua_util.c kendynet.a
	$(CC) $(CFLAGS) -o luanet src/luanet.c src/lua_util.c kendynet.a  $(INCLUDE) $(LDFLAGS)	$(DEFINE) 

	
	
