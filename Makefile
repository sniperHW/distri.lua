all:luanet.c
	gcc -O3 -g -c -fPIC luanet.c -I../kendylib/include
	gcc -O3 -shared -o luanet.so luanet.o kendy.a -lpthread -lrt -ltcmalloc
server:
	gcc -g -c luanet.c -I../kendylib/include
	gcc -g -o server server.c luanet.o kendy.a -llua -lpthread -lrt -ltcmalloc -ldl
client:
	gcc -g -c luanet.c -I../kendylib/include
	gcc -g -o client client.c luanet.o kendy.a -llua -lpthread -lrt -ltcmalloc -ldl	
