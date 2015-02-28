all:producer.c consumer.c
	gcc -g -o consumer consumer.c ../../../libkendynet.a -lrt  -I../../../include -D_LINUX -lpthread
	gcc -g -o producer producer.c ../../../libkendynet.a -lrt  -I../../../include -D_LINUX -lpthread