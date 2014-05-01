proactor模式的网络库,重构自[KendyNet](https://github.com/sniperHW/kendynet)的网络层.
主要调整包括：

    1）调整函数和结构体的命名规范，在函数和结构体前加上kn_前缀
    2) 函数中的栈上变量全部在函数体最开始声明
    3) 添加对udp和unix域套接字的支持