一个轻量级的网络通讯库 


测试用例说明:

testserver.c testclient.c:单向传输测试

testchrdev.c:从标准输入读取输入，发往服务器，服务器回射后输出

testredis.c:redis异步接口测试

testtimer.c:定时器测试

broadcast_cli.c broadcast_svr.c:数据包广播测试

tcpserver.c pingpong.c:pingpong测试

testmailbox.c : 线程邮箱测试

testlog.c : 日志测试

testlua.c test.lua : lua封装接口测试


