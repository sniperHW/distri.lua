一套由lua实现的轻量级分布式系统解决方案,主要特性如下:
    
    1)异步IO.
    
    2)单线程
    
    3)异步日志系统.
    
    4)coroutine化异步为同步
    
    5)支持tcp,udp,unix域套接字等多种通讯协议
    
    6)支持消息方式和RPC方式的通讯
    
    7)淡化连接的概念，各服务之间通过名字通讯
    
 整个框架在通讯方面只暴露3个函数:
 
 	SendMsg(name,msg)
 	向远程服务name发送一条msg，如果与name还没有建立通讯连接，则由SendMsg函数内部向nameservice查询name的信息
 	并建立通讯连接.
 	
 	
	RpcCall(name,funcname,argument)
	向name发起一个远程调用，请求执行funcname方法，如果与name的通讯连接没有建立执行与SendMsg一样的步骤
	
	GetRemoteFuncProvider(funcname)
	返回所有提供funcname远程方法的服务的名字(自己除外)
	
所有的服务启动后首先连接到名字服务,由名字服务分配端口(TCP或UDP)/本地文件地址(unix域套接字),建立监听连接之后将本
服务的名字和所提供的远程方法发往名字服务注册。



	
	
	  

    
