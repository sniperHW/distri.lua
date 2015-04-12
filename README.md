distri.lua
======
distri.lua is a lua concurrency networking framework aid to help the developer to fast
build online game server ,web application,light distributed system and so on.

Features include:

* Fast event loop
* Supported TCP and UDP
* Cooperative socket library
* RPC framework
* Supported Linux and FreeBSD
* Cooperative dns query
* Lighted HttpServer
* SSL Connection
* WebSocket
* local and remote debug of lua
* integrate redis client interface(Synchronous and asynchronous) 

distri.lua is licensed under GPL license.


get distri.lua
-----------

Install libcurl

Install lua 5.2.

Clone [the repository](https://github.com/sniperHW/distri.lua).

Post feedback and issues on the [bug tracker](https://github.com/sniperHW/distri.lua/issues),


build
------
```
in Ubuntu

sudo apt-get install libssl-dev
sudo apt-get install libreadline-dev

make

in FreeBSD

gmake
```

running tests
-------------
```
pingpong test

./distrilua samples/hello.lua
./distrilua samples/pingclient.lua

rpc test

./distrilua samples/rpcserver.lua
./distrilua samples/rpcclient.lua

```


to-do list
----------
* Cooperative dns query
* WebSocket
* debuger

http test
----------

    local Http = require "lua/http"
    Http.CreateServer(function (req, res) 
        res:WriteHead(200,"OK", {"Content-Type: text/plain"})
        res:End("Hello World\n");
        end):Listen("127.0.0.1",8001)

    print("create server on 127.0.0.1 8001")

udp test
-----------

server

	local Socket = require "lua.socket"
	local Timer = require "lua.timer"
	local Sche = require "lua.sche"

	local count = 0
	local s = Socket.Datagram.New(CSocket.AF_INET,1024,Socket.Datagram.RDecoder())
	s:Listen("127.0.0.1",8010)


	Sche.Spawn(function ()
		while true do
			local rpk,from = s:Recv()
			--print(from[1],from[2],from[3])
			count = count + 1
			s:Send(Socket.WPacket(rpk),from)
		end
	end)

	local last = C.GetSysTick()
	local timer = Timer.New():Register(function ()
		local now = C.GetSysTick()
		print(string.format("count:%d",count*1000/(now-last)))
		count = 0
		last = now
	end,1000):Run()

client

	local Socket = require "lua.socket"

	local s = Socket.Datagram.New(CSocket.AF_INET,1024,Socket.Datagram.RDecoder())
	for i = 1,3 do
		local wpk = Socket.WPacket(1024)
		wpk:Write_string("hello")
		s:Send(wpk,{CSocket.AF_INET,"127.0.0.1",8010})
	end

	while true do
		local rpk,from = s:Recv()
		s:Send(Socket.WPacket(rpk),from)
	end

RPC test
-----------

server

	local TcpServer = require "lua.tcpserver"
	local App = require "lua.application"
	local RPC = require "lua.rpc"
	local Timer = require "lua.timer"
	local Sche = require "lua.sche"
	local Socket = require "lua.socket"


	local count = 0

	local rpcserver = App.New()

	rpcserver:RPCService("Plus",function (_,a,b)
		count = count + 1 
		return a+b 
	end)

	local success

	local success = not TcpServer.Listen("127.0.0.1",8000,function (client)
				print("on new client")		
				rpcserver:Add(client:Establish(Socket.Stream.RDecoder()))		
		end)


	if success then
		print("server start on 127.0.0.1:8000")
		local last = C.GetSysTick()
		local timer = Timer.New():Register(function ()
			local now = C.GetSysTick()
			print(string.format("cocount:%d,rpccount:%d,elapse:%d",
					     Sche.GetCoCount(),count*1000/(now-last),now-last))
			count = 0
			last = now
		end,1000):Run()
	else
		print("server start error")
	end

client

	local Socket = require "lua.socket"
	local App = require "lua.application"
	local RPC = require "lua.rpc"
	local Sche = require "lua.sche"

	local rpcclient = App.New()

	for i=1,10 do
		Sche.Spawn(function () 
			local client = Socket.Stream.New(CSocket.AF_INET)
			if client:Connect("127.0.0.1",8000) then
				print("connect to 127.0.0.1:8000 error")
				return
			end		
			rpcclient:Add(client:Establish(Socket.Stream.RDecoder()),nil,on_disconnected)
			local rpcHandler = RPC.MakeRPC(client,"Plus")
			for j=1,100 do
				Sche.Spawn(function (client)
					while true do			
						local err,ret = rpcHandler:Call(1,2)
						if err then
							print("rpc error:" .. err)
							client:Close()
							return
						end
					end
				end,client)
			end
		end)	
	end


test redis client
-----------

	local Redis = require "lua.redis"
	local Sche = require "lua.sche"
	local Timer = require "lua.timer"


	local count = 0
	local toredis

	local function connect_to_redis()
		print("here")
	    if toredis then
			print("to redis disconnected")
	    end
	    toredis = nil
		Sche.Spawn(function ()
			while true do
				local err
				err,toredis = Redis.Connect("127.0.0.1",6379,connect_to_redis)
				if toredis then
					break
				end
				print("try to connect after 1 sec")
				Sche.Sleep(1000)
			end
		end)	
	end

	connect_to_redis()

	while not toredis do
		Sche.Yield()
	end

	local err,result = toredis:CommandSync("hmget test nickname")

	if result then
		for k,v in pairs(result) do
			print(k,v)
		end
	end


demo of online game 
-----------
see [Survive](https://github.com/sniperHW/Survive)
