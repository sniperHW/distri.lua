CFLAGS = -g -Wall -fno-strict-aliasing
LDFLAGS = -lpthread -lrt -lm -lssl -lcrypto
SHARED = -fPIC -shared
CC = gcc
INCLUDE = -I../../include -I./ 
DEFINE = -D_DEBUG -D_LINUX

main:logicprocessor.c ioworker.c main.c
	$(CC) $(CFLAGS) -o main main.c ioworker.c logicprocessor.c ../../libkendynet.a $(INCLUDE)  $(LDFLAGS)