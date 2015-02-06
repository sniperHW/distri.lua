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

main:logicprocessor.c ioworker.c main.c
	$(CC) $(CFLAGS) -o main main.c ioworker.c logicprocessor.c ../../libkendynet.a $(INCLUDE)  $(LDFLAGS) $(DEFINE)