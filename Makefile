PLAT =
CFLAGS = -g -Wall -fno-strict-aliasing
LDFLAGS = -lpthread -lrt -lm -lssl -lcrypto
INCLUDE = -IKendyNet/include -Ideps -Ideps/lua-5.2.3/src 
DEFINE =
BIN = distrilua
LIB = KendyNet/libkendynet.a deps/hiredis/libhiredis.a deps/http-parser/libhttp_parser.a deps/lua-5.2.3/src/liblua.a
DEPENDENCY = KendyNet/libkendynet.a  deps/hiredis/libhiredis.a deps/http-parser/libhttp_parser.a deps/lua-5.2.3/src/liblua.a  cjson.so

source = src/luasocket.c\
	  src/luapacket.c\
	  src/luaredis.c\
	  src/luahttp.c\
	  src/lualog.c\
	  src/base64/base64.c\
	  src/base64/lua_base64.c\
	  src/timeutil.c\
	  src/listener.c\
	  src/distri.c

# Platform-specific overrides
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(uname_S),Linux)
	LDFLAGS += -ldl
	DEFINE += -D_LINUX
	PLAT += linux
	source += src/Hook.c src/debug.c  
	LIB += deps/jemalloc/lib/libjemalloc.a deps/myprocps/libproc.a
	DEPENDENCY += deps/jemalloc/lib/libjemalloc.a deps/myprocps/libproc.a
endif

ifeq ($(uname_S),FreeBSD)
	LDFLAGS += -lexecinfo
	DEFINE += -D_BSD
	PLAT += bsd
endif

all: $(DEPENDENCY) $(source)	
	$(CC) $(CFLAGS)  -o $(BIN) $(source) $(LIB) $(INCLUDE) $(LDFLAGS) $(DEFINE) -rdynamic


KendyNet/libkendynet.a:
		cd KendyNet;make
deps/jemalloc/lib/libjemalloc.a:
		cd deps/jemalloc;./configure;make
deps/hiredis/libhiredis.a:
		cd deps/hiredis/;make $(PLAT)

deps/http-parser/libhttp_parser.a:		
		cd deps/http-parser/;make package
		
deps/lua-5.2.3/src/liblua.a:		
		cd deps/lua-5.2.3/;make $(PLAT)

deps/myprocps/libproc.a:
		cd deps/myprocps/;make							
cjson.so:
		cd deps/lua-cjson-2.1.0;make
		mv deps/lua-cjson-2.1.0/cjson.so ./	