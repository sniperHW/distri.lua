一套由lua实现的轻量级分布式系统解决方案,主要特性如下:
    
    1)异步IO.
    
    2)单线程
    
    3)异步日志系统.
    
    4)通过coroutine化异步为同步
    
    5)支持tcp,udp,unix域套接字等多种通讯协议
    
    6)支持消息方式和RPC方式的通讯
    
    7)通过名字实现进程间的通信


luanet分布式集群按功能分为nameservice和应用服务器.nameservice在集群中唯一部署,用于按名字获取服务
监听的网络协议以及监听地址.

应用服务器在启动后首先连接到nameservice,向nameservice注册自己的唯一名字,由nameservice为服务根据注册的
协议分配监听端口号(对于TCP和UDP)/地址(unix域协议).

应用服务器之间的通信提供两种方式:
    
    send(name,data)
    
    send_and_waitresponse(name,data)

其中第一种用于普通消息通信，第二种用于rpc通信.

sendX函数内部的工作方式如下:

    1)在本地查找name对应的连接,如果找到跳到 4)
    
    2)向nameservice发送查询消息，查询name的监听地址
    
    3)根据nameservice返回的信息与name建立连接(流式协议).
    
    4)向name发送消息
    
