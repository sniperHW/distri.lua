CFLAGS = -g -Wall -fno-strict-aliasing
LDFLAGS = -lpthread -lrt -lm -lssl -lcrypto
SHARED   =   -fPIC
INCLUDE = -I../../include -I./
DEFINE =

# Platform-specific overrides
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(uname_S),Linux)
	LDFLAGS += -ldl
	DEFINE += -D_LINUX
endif

ifeq ($(uname_S),FreeBSD)
	LDFLAGS += -lexecinfo
	DEFINE += -D_BSD
endif

ut_socket.o:ut_socket.c
	$(CC) $(CFLAGS) -c ut_socket.c $(INCLUDE) $(DEFINE)

server:ut_socket.o server.c
	$(CC) $(CFLAGS) -o server server.c ut_socket.o ../../libkendynet.a $(INCLUDE)  $(LDFLAGS) $(DEFINE)

client:ut_socket.o client.c
	$(CC) $(CFLAGS) -o client client.c ut_socket.o ../../libkendynet.a $(INCLUDE)  $(LDFLAGS) $(DEFINE)	 