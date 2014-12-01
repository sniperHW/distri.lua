CFLAGS = -g -Wall -fno-strict-aliasing -rdynamic 
LDFLAGS = -lpthread -lrt -llua -lm -ldl 
SHARED = -fPIC -shared
CC = gcc
DEFINE = -D_DEBUG -D_LINUX 
INCLUDE = -Ideps/KendyNet/include -Ideps -Ideps/lua-5.2.3/src 
LIB = -L deps/jemalloc/lib -L deps/KendyNet -L deps/hiredis/ -L deps/http-parser -L deps/lua-5.2.3/src -L deps/myprocps/
		
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
		
deps/lua-5.2.3/liblua.a:		
		cd deps/lua-5.2.3/;make linux					
cjson.so:
		cd deps/lua-cjson-2.1.0;make
		mv deps/lua-cjson-2.1.0/cjson.so ./

source =  src/luasocket.c\
	  src/luapacket.c\
	  src/luaredis.c\
	  src/luahttp.c\
	  src/lualog.c\
	  src/Hook.c \
	  src/debug.c \
	  src/base64/base64.c\
	  src/base64/lua_base64.c\
	  src/distri.c 		

distrilua: deps/KendyNet/libkendynet.a\
	  deps/jemalloc/lib/libjemalloc.a\
	  deps/hiredis/libhiredis.a\
	  deps/http-parser/libhttp_parser.a\
	  deps/lua-5.2.3/liblua.a\
	  cjson.so\
	  $(source)	
	$(CC) $(CFLAGS) $(LIB) -o distrilua $(source) -lkendynet deps/hiredis/libhiredis.a -lhttp_parser deps/jemalloc/lib/libjemalloc.a deps/myprocps/libproc.a $(INCLUDE) $(LDFLAGS) $(DEFINE) $(LIB)
