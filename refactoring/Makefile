CFLAGS = -g -Wall -fno-strict-aliasing -rdynamic 
LDFLAGS = -lpthread -lrt -llua5.2 -lm -ldl -ltcmalloc 
SHARED = -fPIC -shared
CC = gcc
DEFINE = -D_DEBUG -D_LINUX 
		
test:test.c
	$(CC) $(CFLAGS) -o test test.c $(LDFLAGS) $(DEFINE)
	
testusrdata:testusrdata.c
	$(CC) $(CFLAGS) -o testusrdata testusrdata.c $(LDFLAGS) $(DEFINE) 	 

	
	
