server:../lib/kendylib.a test.c
	gcc -g -o server test.c ../lib/kendylib.a  -I.. -lpthread -lrt -D _LINUX -ltcmalloc
connect:../lib/kendylib.a testconnect.c
	gcc -g -o connect_test testconnect.c ../lib/kendylib.a  -I.. -lpthread -lrt -D _LINUX -ltcmalloc
tcpserver:../..//kendylib.a benchserver.c
	gcc -g -o tcpserver benchserver.c ../../kendylib.a  -I.. -lpthread -lrt -D _LINUX -ltcmalloc
tcpclient:../../kendylib.a benchclient.c
	gcc -g -o tcpclient benchclient.c ../../kendylib.a  -I.. -lpthread -lrt -D _LINUX -ltcmalloc
timer:../../kendylib.a testtimer.c
	gcc -g -o timer testtimer.c ../../kendylib.a  -I.. -lpthread -lrt -D _LINUX -ltcmalloc
msgque:../lib/kendylib.a testmq.c
	gcc -g -o msgque testmq.c ../lib/kendylib.a  -I.. -lpthread -lrt -D _LINUX -ltcmalloc
systick:../lib/kendylib.a testgetsystick.c
	gcc -g -o systick testgetsystick.c ../lib/kendylib.a  -I.. -lpthread -lrt -D _LINUX -ltcmalloc

