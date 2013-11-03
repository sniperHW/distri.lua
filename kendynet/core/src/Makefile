debug:
	gcc -g -c *.c -I../ -D _DEBUG -D _LINUX
	ar -rc kendylib.a *.o
	mv kendylib.a ../../lib
	rm -f *.o
release:
	gcc -O3 -g -c *.c -I../ -D _DEBUG -D _LINUX
	ar -rc kendylib.a *.o
	mv kendylib.a ../lib
	rm -f *.o
