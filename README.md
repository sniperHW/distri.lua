一套由lua实现的轻量级分布式系统解决方案,主要特性如下:
    
    1)异步IO.
    
    2)异步日志系统.
    
    3)coroutine化异步为同步
    
    4)支持tcp,udp,unix域套接字等多种通讯协议
    
    5)支持消息方式和RPC方式的通讯
    
    6)淡化连接的概念，各服务之间通过名字通讯
    
 整个框架在通讯方面只暴露4个函数:
 
 	SendMsg(name,msg)
 	向远程服务name发送一条msg，如果与name还没有建立通讯连接，则由SendMsg函数内部向nameservice查询name的信息
 	并建立通讯连接.
 	
 	
	RpcCall(name,funcname,argument)
	向name发起一个远程调用，请求执行funcname方法，如果与name的通讯连接没有建立执行与SendMsg一样的步骤
	
	GetRemoteFuncProvider(funcname)
	返回所有提供funcname远程方法的服务的名字(自己除外)
	
	GetMsg()
	获取网络过来的普通消息
	
###安装

		make luanet

	
###性能测试

测试环境 intel-i5 双核 2.53HZ 服务器客户端均在本机运行

测试内容:echo回射,每个包的字节数在20字节内

[luanet](https://github.com/sniperHW/luanet)
 
	连接数    每秒回射数			
	1         19,000/s
	10        12,5000/s
	100       12,0000/s
	1000      80,000/s 

[node.js](http://nodejs.org/)

	连接数    每秒回射数			
	1         27,000/s
	10        30,000/s
	100       30,000/s
	1000      27,000/s

[luvit](http://luvit.io/)

	连接数    每秒回射数			
	1         16,500/s
	10        74,000/s
	100       75,000/s
	1000      51,000/s


从测试结果上看只有在1个连接的情况下luanet不如node.js,当连接数上去之后
luanet每秒的回射数基本都在node.js的3倍左右.在所有的连接数下都比luvit
高30%以上.

node.js:echo.js

		var net = require('net');
		var server = net.createServer(function(c) { // 'connection' 监听器
		  console.log('一个新连接');
		  c.on('end', function() {
			console.log('连接断开');
		  });
		  c.on('data',function(data){
			c.write(data);
		  });
		  c.on('close',function(){
			  console.log('连接断开');
		  });   
		  c.on('error',function(e){
		  });  
		});
		server.listen(8010, function() { // 'listening' 监听器
		  console.log('服务器监听8010');
		});

luvit:echo.lua

	local net = require('net')

	net.createServer(function (client)
	  -- Echo everything the client says back to itself
	  client:pipe(client)
	end):listen(8010)

	print("TCP echo server listening on port 8010")

luanet:echoserver.lua

	local cjson = require "cjson"

	function on_data(s,data,err)
		if not data then
			print("a client disconnected")
			C.close(s)
		else
			local tb = cjson.decode(data)
			C.send(s,cjson.encode(tb),nil)
		end
	end

	function on_newclient(s)
		print("on_newclient")
		if not C.bind(s,{recvfinish = on_data})then
			print("bind error")
			C.close(s)
		end
	end

	C.listen(IPPROTO_TCP,SOCK_STREAM,net.netaddr_ipv4("127.0.0.1",8010),
	         {onaccept=on_newclient})

统一测试客户端,echoclient.lua

	local net = require "lua/netaddr"
	local cjson = require "cjson"
	local Sche = require "lua/scheduler"
	local count = 0

	function on_data(s,data,err)
		if not data then
			print("a client disconnected")
			C.close(s)
		else
			count = count + 1
			local tb = cjson.decode(data)
			C.send(s,cjson.encode(tb),nil)
		end
	end

	function on_connected(s,remote_addr,err)
		print("on_connected")
		if s then
			if not C.bind(s,{recvfinish = on_data}) then
				print("bind error")
				C.close(s)
			else
				print("bind success")
				C.send(s,cjson.encode({"hahaha"}),nil)
			end
		end	
	end
	print("echoclient")
	for i=1,1 do
		C.connect(IPPROTO_TCP,SOCK_STREAM,net.netaddr_ipv4("127.0.0.1",8010),
			      nil,{onconnected = on_connected},3000)
	end

	local tick = C.GetSysTick()
	local now = C.GetSysTick()
	while true do 
		now = C.GetSysTick()
		if now - tick >= 1000 then
			print(count*1000/(now-tick) .. " " .. now-tick)
			tick = now
			count = 0
		end
		Sche.Sleep(50)
	end

luanet rpc测试：客户端调用服务端的Plus函数,函数只是把客户端提供的两个参数相加并返回

平均每秒rpc调用次数在6,8000左右.而用C+协程实现的版本在70,0000左右.我试着用luajit来运行
同样的测试,非常意外的是性能差了一大截,只有可怜的4,000次,具体原因还在调查中.

测试代码:[server.lua](https://github.com/sniperHW/distri.lua/blob/master/server.lua),[client.lua](https://github.com/sniperHW/distri.lua/blob/master/client.lua)

###后续开发计划:

+ 增加新的二进制序列化机制

+ 异步日志系统

+ 异步磁盘文件IO

+ 多线程支持(通过Fork API创建一个新线程运行独立的虚拟机,各线程间通过channel通信)

+ 异步数据库访问(通过线程模块实现) 

+ 安全套接字的支持

+ 跨平台

+ websocket支持





	




	
	
	  

    
