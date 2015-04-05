#distri.lua使用引导

目录

> [1 概述](#1 概述)

> [2 安装distri.lua](#2 安装distri.lua)

> [3 事件处理模型](#3 事件处理模型)

> [4 协程的使用](#4 协程的使用)

> [5 网络编程](#5 网络编程)
>
>- [5.1 socket接口](#5.1 socket接口)
>- [5.2 封包处理](#5.2 封包处理)
>- [5.3 application](#5.3 application) 
>- [5.4 RPC](#5.4 RPC)   

> [6 redis接口](#7 redis接口)

> [7 定时器](#8 定时器)

> [8 通过C模块扩展](#9 通过C模块扩展)

> [9 一个小型手游服务端示例](#10 一个小型手游服务端示例)

###<span id="1 概述">1 概述</span>

distri.lua是一个轻量级的lua网络应用框架,其主要设计目标是使用lua语言快速开发小型的分布式系统,网络游戏服务端程序,web应用等.distri.lua的特点:

* 快速高效的网络模型
* 基于lua协程的并发处理
* 提供了同步的RPC调用接口
* 单线程(除日志线程)
* 内置同步的redis访问接口
* http协议处理
* 内置libcurl支持
* 目前只支持64位linux系统


###<span id="2 安装distri.lua">2 安装distri.lua</span>

[从github上克隆项目的最新版本](https://github.com/sniperHW/distri.lua.git)

在ubuntu下首先使用以下命令安装依赖库

sudo apt-get install libssl-dev

sudo apt-get install libreadline-dev

其它系统请自行查找安装方法

执行make

执行./distrilua examples/testsche 确定已经编译成功

###<span id="3 事件处理模型">4 事件处理模型</span>

distri.lua使用KendyNet处理网络事件.KendyNet是一个将网络事件,定时器事件,线程队列消息,redis消息,libcurl消息,统一到一个消息分发器中的单线程事件处理系统.由使用者根据不同的接口注册不同的事件处理函数,当事件到达时自动根据事件类型触发合适的回调.通过使用统一的事件循环处理各种不同的消息,大大减少了应用程序的编写难度,提高了系统的并发性.


###<span id="4 协程的使用">4 协程的使用</span>

distri.lua是一个完全基于lua协程实现的系统,使用者编写的所有逻辑代码都运行在协程环境下,所以在任何地方都可以方便的使用调度器提供的接口将当前协程阻塞而不会阻塞运行线程。系统同时将网络事件,redis操作等接口通过协程的封装实现了一套同步的访问接口,使用者可以方便的通过同步代码享受到异步处理带来的高效率.(对比node.js的回调地狱)

协程相关接口在lua.sche包中,lua.sche包提供了以下API:

* Spawn(func,...):创建一个协程,并运行func函数,协程将会在调度器下一次调度是开始运行

* SpawnAndRun(func,...):与Spawn类似,但协程立即开始执行

* Running():取得当前正在运行的协程对象

* Sleep(ms):让当前协程休眠ms毫秒(设置stat_sleep状态)

* Yield():当前协程放弃运权,让调度器调度其它协程,当前协程将会被插入到可运行队列的尾部

* Block(ms):使得当前协程阻塞,如果ms非空且>0则最多阻塞ms毫秒(设置stat_block状态)

* Schedule(co):如果co为空则执行调度器,否则将执行权切换到co

* Exit():终止调度器,同时导致程序终止    

下面的一段代码展示了协程的使用:

    local Sche = require "lua.sche"

    local function co_fun(name)
        while true do
            print(name)
            Sche.Sleep(1000)
        end
    end

    Sche.Spawn(co_fun,"co1")
    Sche.Spawn(co_fun,"co2")
    Sche.Spawn(co_fun,"co3")
    Sche.Spawn(co_fun,"co4")
    
这段代码创建了4个协程,分别运行主函数co_fun,从输出到可以看出4个协程是在并发运行的,Sleep(1000)并没有阻塞当前运行线程.    
    
###<span id="5 网络编程">5 网络编程</span>

####<span id="5.1 socket接口">5.1 socket接口</span>

socket包中包含了distri.lua提供的基础网络编程接口,下面是基本API的介绍:

* socket.New(domain,type,protocol):用于创建套接口对象供后续使用.目前3个参数分别仅支持CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP.

* socket.Close():销毁套接口对象,如果连接建立会导致连接关闭,当一个socket对象不再需要时,务必确保对其调用socket.Close()

* socket.Listen(ip,port):建立监听

* socket.Accept():接受一个连接,如果没有连接到达,当前协程会被阻塞,直到有一个连接到达

* socket.Connect(ip,port):请求建立一个到远端的连接,当前协程会被阻塞,直到连接建立或出错

* socket.Establish(decoder,recvbuf_size):建立通信信道,Connect成功和用Accept接受到的套接口在调用实际的IO函数之前首先需要调用Establish函数.其中decoder是一个解包器,目前支持的解包器类型为,rpacket,rawpacket和httppacket(解包器将在后面介绍).recvbuf_size是底层接收缓冲的大小.

* socket.Recv(timeout):接收并返回一个完整的封包.如果当前没有完整的封包,当前协程会被阻塞,直到有一个完整封包到达/超时/连接断开.

* socket.Send(packet):向对端发送一个封包.

socket接口的使用可以参看示例examples/hello2.lua,这个示例实现了一个简单的echo服务.启动之后可以直接telnet上去查看运行结果.

####<span id="5.2 封包处理">5.2 封包处理</span>

distri.lua内置了3种类型的封包:packet,rawpacket和httppacket.

其中packet和rawpacket的是二进制的字节流封包,packet与rawpacket的区别在于,rawpacket保存的是原始的二进制数据,而packet包含一个4字节的长度字段,并且提供了一组函数用于向封包中写入/读出各种类型的数据.

packet分成两种类型分别为wpacket和rpacket,分别用于读和写,下面通过一个例子介绍packet,rawpacket的使用方法:

    local wpk = CPacket.NewWPacket() 		--创建一个空的wpacket
    wpk:Write_uint32(100)            		--写入uint32   
	wpk:Write_uint16(10)      				--写入uint16
    wpk:Write_string("hello")               --写入string 
    local rpk = CPacket.NewRPacket(wpk)     --从wpacket的数据构造rpacket 

    print(rpk:Read_uint32())                --读取uint32 
    print(rpk:Peek_uint16())				--peek uint16,不改变读位置,后面可以继续读到此数据
    print(rpk:Read_uint16())                --读取uint16   
    print(rpk:Read_string())                --读取string

    --[[
    100
    10
    10
    hello
    ]]--

    wpk = CPacket.NewWPacket()
    local wpos = wpk:Get_write_pos()       --获得当前写入位置 
    wpk:Write_uint8(0)                     --写入uint8 
    wpk:Write_uint32(100)                  --写入uint32
    wpk:Rewrite_uint8(wpos,255)			   --将第一个uint8改成255 	
    rpk = CPacket.NewRPacket(wpk)

    print(rpk:Reverse_read_uint32())       --反向读取uint32,不会导致读位置的变更
    print(rpk:Read_uint8())                --读取uint8
    print(rpk:Read_uint32())               --读取uint32

    --[[
    100
    255
    100
    ]]--


    local rawpk = CPacket.NewRawPacket("hello")  --创建一个rawpacket,数据为hello
    print(rawpk:Read_rawbin())                   --从rawpacket中读取数据

    --[[
    hello
    ]]--

除了基本类型以外,wpacket和rpacket还支持对lua table的打包和解包操作,因此可以直接使用lua table作为网络通讯协议,
以下代码片段展示了如何操作lua table:

    local t = {
    	65537,
    	"hello",
    	{-3,-65536},
    	fuck = "you",
                  hello = function () print("hello") end
    }
    --setmetatable(t,{})
    
    local wpk = CPacket.NewWPacket(512)
    
    wpk:Write_table(t)
    local tt = CPacket.NewRPacket(wpk):Read_table()
    
    print(tt[1])
    print(tt[2])
    print(tt[3][1])
    print(tt[3][2])
    print(tt.fuck)
    print(tt.hello)


####<span id="5.3 application">5.3 application</span>

####<span id="5.4 RPC">5.4 RPC</span>
