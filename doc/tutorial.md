#distri.lua使用引导

目录

* [概述](#概述)
* [安装distri.lua](#安装distri.lua)
* [事件处理模型](#事件处理模型)
* [协程的使用](#协程的使用)
* [网络编程](#网络编程)
* [RPC](#RPC)
* [redis接口](#redis接口)
* [定时器](#定时器)
* [通过C模块扩展](#通过C模块扩展)
* [一个小型手游服务端示例](#一个小型手游服务端示例)

###<span id="概述">概述</span>

distri.lua是一个轻量级的lua网络应用框架,其主要设计目标是使用lua语言快速开发小型的分布式系统,网络游戏服务端程序,web应用等.distri.lua的特点:

* 快速高效的网络模型
* 基于lua协程的并发处理
* 提供了同步的RPC调用接口
* 单线程(除日志线程)
* 内置同步的redis访问接口
* http协议处理
* 内置libcurl接口
* 目前只支持64位linux系统


###<span id="安装distri.lua">安装distri.lua</span>

[从github上克隆项目的最新版本](https://github.com/sniperHW/distri.lua.git)

[进入deps目录克隆KendyNet](https://github.com/sniperHW/KendyNet.git)

执行make distrilua

执行./distrilua examples/testsche 确定已经成功安装

###<span id="事件处理模型">事件处理模型</span>

distri.lua使用KendyNet处理网络事件.KendyNet是一个将网络事件,定时器事件,线程队列消息,redis消息,libcurl消息,统一到一个消息分发器中的单线程事件处理系统.由使用者根据不同的接口注册不同的事件处理函数,当事件到达时自动根据事件类型触发合适的回调.通过使用统一的事件循环处理各种不同的消息,大大减少了应用程序的编写难度,提高了系统的并发性.


###<span id="协程的使用">协程的使用</span>

distri.lua是一个完全基于lua协程实现的系统,使用者编写的所有逻辑代码都运行在协程环境下,所以在任何地方都可以方便的使用调度器提供的接口将当前协程阻塞而不会阻塞运行线程。系统同时将网络事件,redis操作等接口通过协程的封装实现了一套同步的访问接口,使用者可以方便的通过同步代码享受到异步处理带来的高效率.(对比node.js的回调地狱)

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
    
###<span id="网络编程">网络编程</span>    