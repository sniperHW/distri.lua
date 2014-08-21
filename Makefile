CFLAGS = -g -Wall -fno-strict-aliasing -rdynamic 
LDFLAGS = -lpthread -lrt -llua5.2 -lm -ldl -ltcmalloc
SHARED = -fPIC -shared
CC = gcc
DEFINE = -D_DEBUG -D_LINUX 
INCLUDE = -Ideps/KendyNet/include -Ideps -I/usr/include/lua5.2 
LIB = -Ldeps/jemalloc/lib -Ldeps/KendyNet -Ldeps/hiredis/
		
test:test.c
	$(CC) $(CFLAGS) -o test test.c $(LDFLAGS) $(DEFINE)
	
testusrdata:testusrdata.c
	$(CC) $(CFLAGS) -o testusrdata testusrdata.c $(LDFLAGS) $(DEFINE) 
	
deps/KendyNet/kendynet.a:
		cd deps/KendyNet;make kendynet.a
deps/jemalloc/lib/libjemalloc.a:
		cd deps/jemalloc;./configure;make
deps/hiredis/libhiredis.a:
		cd deps/hiredis/;make
cjson.so:
		cd deps/lua-cjson-2.1.0;make
		mv deps/lua-cjson-2.1.0/cjson.so ./

distrilua:deps/KendyNet/kendynet.a\
		  deps/jemalloc/lib/libjemalloc.a\
		  deps/hiredis/libhiredis.a\
		  cjson.so\
		  src/luasocket.h\
		  src/luasocket.c\
		  src/luabytebuffer.h\
		  src/luabytebuffer.c\
		  src/distri.c	
		$(CC) $(CFLAGS) -o distrilua src/distri.c src/luasocket.c src/luabytebuffer.c kendynet.a deps/hiredis/libhiredis.a $(LIB) $(INCLUDE) $(LDFLAGS) $(DEFINE)
