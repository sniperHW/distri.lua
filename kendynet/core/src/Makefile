debug:
	gcc -g -c *.c -I../ -D _DEBUG -D _LINUX
	ar -rc kendynet.a *.o
	mv kendynet.a ../../../
	rm -f *.o
release:
	gcc -O3 -g -c *.c -I../ -D _DEBUG -D _LINUX
	ar -rc kendynet.a *.o
	mv kendynet.a ../../../
	rm -f *.o
