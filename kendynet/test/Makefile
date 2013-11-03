server:../lib/kendylib.a test.c
	gcc -g -o server test.c ../lib/kendylib.a  -I.. -lpthread -lrt -D _LINUX -ltcmalloc
connect:../lib/kendylib.a testconnect.c
	gcc -g -o connect_test testconnect.c ../lib/kendylib.a  -I.. -lpthread -lrt -D _LINUX -ltcmalloc
tcpserver:../lib/kendylib.a netboradcast.c
	gcc -g -o tcpserver netboradcast.c ../lib/kendylib.a  -I.. -lpthread -lrt -D _LINUX -ltcmalloc
tcpclient:../lib/kendylib.a netclient.c
	gcc -g -o tcpclient netclient.c ../lib/kendylib.a  -I.. -lpthread -lrt -D _LINUX -ltcmalloc
timer:../lib/kendylib.a testtimer.c
	gcc -g -o timer testtimer.c ../lib/kendylib.a  -I.. -lpthread -lrt -D _LINUX -ltcmalloc
msgque:../lib/kendylib.a testmq.c
	gcc -g -o msgque testmq.c ../lib/kendylib.a  -I.. -lpthread -lrt -D _LINUX -ltcmalloc
systick:../lib/kendylib.a testgetsystick.c
	gcc -g -o systick testgetsystick.c ../lib/kendylib.a  -I.. -lpthread -lrt -D _LINUX -ltcmalloc

