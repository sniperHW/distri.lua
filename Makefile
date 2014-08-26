CFLAGS = -g -Wall -fno-strict-aliasing -rdynamic 
LDFLAGS = -lpthread -lrt -llua5.2 -lm -ldl 
SHARED = -fPIC -shared
CC = gcc
DEFINE = -D_DEBUG -D_LINUX 
INCLUDE = -Ideps/KendyNet/include -Ideps -I/usr/include/lua5.2 
LIB = -L deps/jemalloc/lib -L deps/KendyNet -L deps/hiredis/ -L deps/http-parser
		
test:test.c
	$(CC) $(CFLAGS) -o test test.c $(LDFLAGS) $(DEFINE)
	
testusrdata:testusrdata.c
	$(CC) $(CFLAGS) -o testusrdata testusrdata.c $(LDFLAGS) $(DEFINE) 
	
deps/KendyNet/libkendynet.a:
		cd deps/KendyNet;make libkendynet.a
deps/jemalloc/lib/libjemalloc.a:
		cd deps/jemalloc;./configure;make
deps/hiredis/libhiredis.a:
		cd deps/hiredis/;make

deps/http-parser/libhttp_parser.a:		
		cd deps/http-parser/;make package		
cjson.so:
		cd deps/lua-cjson-2.1.0;make
		mv deps/lua-cjson-2.1.0/cjson.so ./

distrilua:deps/KendyNet/libkendynet.a\
		  deps/jemalloc/lib/libjemalloc.a\
		  deps/hiredis/libhiredis.a\
		  deps/http-parser/libhttp_parser.a\
		  cjson.so\
		  src/luasocket.h\
		  src/luasocket.c\
		  src/luapacket.h\
		  src/luapacket.c\
		  src/luahttp.c\
		  src/distri.c	
		$(CC) $(CFLAGS) $(LIB) -o distrilua src/distri.c src/luasocket.c src/luapacket.c src/luahttp.c -lkendynet -lhiredis -lhttp_parser deps/jemalloc/lib/libjemalloc.a $(INCLUDE) $(LDFLAGS) $(DEFINE)
