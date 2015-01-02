CFLAGS = -g -Wall -fno-strict-aliasing
LDFLAGS = -lpthread -lrt -lm
SHARED = -fPIC -shared
CC = gcc
INCLUDE = -I../../include -I./ 
DEFINE = -D_DEBUG -D_LINUX

all:ut_socket.c main.c
	$(CC) $(CFLAGS) -o main main.c ut_socket.c ../../libkendynet.a $(INCLUDE)  $(LDFLAGS)