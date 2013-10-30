all:luanet.c
	gcc -g -c -fPIC luanet.c -I../purenet/
	gcc -g -shared -o luanet.so luanet.o ../purenet/lib/kendylib.a -lpthread -lrt -ltcmalloc
server:
	gcc -g -c luanet.c -I../kendylib/include
	gcc -g -o server server.c luanet.o kendy.a -llua -lpthread -lrt -ltcmalloc -ldl
client:
	gcc -g -c luanet.c -I../kendylib/include
	gcc -g -o client client.c luanet.o kendy.a -llua -lpthread -lrt -ltcmalloc -ldl	
