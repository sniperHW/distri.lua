all:luanet.c
	gcc -g -c -fPIC luanet.c -I../kendylib/include
	gcc -shared -o luanet.so luanet.o kendy.a -lpthread -lrt
