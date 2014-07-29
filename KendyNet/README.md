[KendyNet](https://github.com/sniperHW/KendyNet)的重构。力求使得框架更加灵活，减少代码的臃肿。


测试用例说明:

testserver.c testclient.c:单向传输测试

testchrdev.c:从标准输入读取输入，发往服务器，服务器回射后输出

testredis.c:redis异步接口测试

testtimer.c:定时器测试