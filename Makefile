PLAT= none
PLATS= bsd linux
CFLAGS = -g -Wall -fno-strict-aliasing
LDFLAGS = -lpthread -lrt -lm -lssl -lcrypto
INCLUDE = -IKendyNet/include -Ideps -Ideps/lua-5.2.3/src 

source = src/luasocket.c\
	  src/luapacket.c\
	  src/luaredis.c\
	  src/luahttp.c\
	  src/lualog.c\
	  src/Hook.c \
	  src/debug.c \
	  src/base64/base64.c\
	  src/base64/lua_base64.c\
	  src/timeutil.c\
	  src/listener.c\
	  src/distri.c

KendyNet/libkendynet.a:
		cd KendyNet;make linux
deps/jemalloc/lib/libjemalloc.a:
		cd deps/jemalloc;./configure;make
deps/hiredis/libhiredis.a:
		cd deps/hiredis/;make linux

deps/http-parser/libhttp_parser.a:		
		cd deps/http-parser/;make package
		
deps/lua-5.2.3/src/liblua.a:		
		cd deps/lua-5.2.3/;make linux

deps/myprocps/libproc.a:
		cd deps/myprocps/;make linux							
cjson.so:
		cd deps/lua-cjson-2.1.0;make
		mv deps/lua-cjson-2.1.0/cjson.so ./

linux:KendyNet/libkendynet.a\
        deps/jemalloc/lib/libjemalloc.a\
        deps/hiredis/libhiredis.a\
        deps/http-parser/libhttp_parser.a\
        deps/lua-5.2.3/src/liblua.a\
        deps/myprocps/libproc.a\
        cjson.so\
        $(source)	
	$(CC) $(CFLAGS) $(LIB) -o distrilua $(source) KendyNet/libkendynet.a \
	deps/hiredis/libhiredis.a \
	deps/jemalloc/lib/libjemalloc.a \
	deps/myprocps/libproc.a \
	deps/http-parser/libhttp_parser.a \
	deps/lua-5.2.3/src/liblua.a \
	$(INCLUDE) $(LDFLAGS) $(DEFINE) -rdynamic -ldl -D_LINUX

bsd:KendyNet/libkendynet.a\
        deps/hiredis/libhiredis.a\
        deps/http-parser/libhttp_parser.a\
        deps/lua-5.2.3/src/liblua.a\
        cjson.so\
        $(source)
	$(CC) $(CFLAGS) $(LIB) -o distrilua $(source) KendyNet/libkendynet.a \
	deps/hiredis/libhiredis.a \
	deps/http-parser/libhttp_parser.a \
	deps/lua-5.2.3/src/liblua.a \
	$(INCLUDE) $(LDFLAGS) $(DEFINE) $(LIB) -rdynamic -lexecinfo

none:
	@echo "Please do 'make PLATFORM' where PLATFORM is one of these:"
	@echo "   $(PLATS)"	