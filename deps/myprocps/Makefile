CC = gcc
INCLUDE = -I proc

linux:
	cc -D_GNU_SOURCE $(INCLUDE) -MM -MG proc/alloc.c proc/devname.c proc/escape.c proc/ksym.c proc/pwcache.c proc/readproc.c proc/sig.c proc/slab.c proc/sysinfo.c proc/version.c proc/whattime.c -D_LINUX > proc/.depend
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/alloc.c -o proc/alloc.o -D_LINUX
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/devname.c -o proc/devname.o -D_LINUX
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/escape.c -o proc/escape.o -D_LINUX
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/ksym.c -o proc/ksym.o -D_LINUX
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/pwcache.c -o proc/pwcache.o -D_LINUX
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/readproc.c -o proc/readproc.o -D_LINUX
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/sig.c -o proc/sig.o -D_LINUX
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/slab.c -o proc/slab.o -D_LINUX
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/sysinfo.c -o proc/sysinfo.o -D_LINUX
	cc -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) -DVERSION=\"3\" -DSUBVERSION=\"2\" -DMINORVERSION=\"8\" -c -o proc/version.o proc/version.c
	cc -D_GNU_SOURCE $(INCLUDE) $(CFLAGS) -c -o top.o top.c -D_LINUX	
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/whattime.c -o proc/whattime.o -D_LINUX	
	ar -rc libproc.a proc/*.o top.o
	cc -g -o testtop testtop.c libproc.a -D_LINUX

bsd:
	cc -D_GNU_SOURCE $(INCLUDE) -MM -MG proc/alloc.c proc/devname.c proc/escape.c proc/ksym.c proc/pwcache.c proc/readproc.c proc/sig.c proc/slab.c proc/sysinfo.c proc/version.c proc/whattime.c -D_BSD > proc/.depend
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/alloc.c -o proc/alloc.o -D_BSD
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/devname.c -o proc/devname.o -D_BSD
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/escape.c -o proc/escape.o -D_BSD
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/ksym.c -o proc/ksym.o -D_BSD
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/pwcache.c -o proc/pwcache.o -D_BSD
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/readproc.c -o proc/readproc.o -D_BSD
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/sig.c -o proc/sig.o -D_BSD
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/slab.c -o proc/slab.o -D_BSD
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/sysinfo.c -o proc/sysinfo.o -D_BSD
	cc -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) -DVERSION=\"3\" -DSUBVERSION=\"2\" -DMINORVERSION=\"8\" -c -o proc/version.o proc/version.c
	cc -D_GNU_SOURCE $(INCLUDE) $(CFLAGS) -c -o top.o top.c -D_BSD	
	cc -c -D_GNU_SOURCE $(INCLUDE)  $(CFLAGS) proc/whattime.c -o proc/whattime.o -D_BSD	
	ar -rc libproc.a proc/*.o top.o
	cc -g -o testtop testtop.c libproc.a -D_BSD	

	
