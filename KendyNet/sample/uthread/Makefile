CFLAGS = -g -Wall -fno-strict-aliasing
LDFLAGS = -lpthread -lrt -lm
SHARED = -fPIC -shared
CC = gcc
INCLUDE = -I../../include -I./ 
DEFINE = -D_DEBUG -D_LINUX

ut_socket.o:ut_socket.c
	$(CC) $(CFLAGS) -c ut_socket.c $(INCLUDE)

server:ut_socket.o server.c
	$(CC) $(CFLAGS) -o server server.c ut_socket.o ../../libkendynet.a $(INCLUDE)  $(LDFLAGS)

client:ut_socket.o client.c
	$(CC) $(CFLAGS) -o client client.c ut_socket.o ../../libkendynet.a $(INCLUDE)  $(LDFLAGS)	