all:shmserver.c shmclient.c
	gcc -O2 -g -o shmserver shmserver.c ../../../libkendynet.a -lrt  -I../../../include -D_LINUX -lpthread
	gcc -O2 -g -o shmclient shmclient.c ../../../libkendynet.a -lrt -I../../../include  -D_LINUX -lpthread

