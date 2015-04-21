local TcpServer = require "lua.tcpserver"
local App = require "lua.application"
local Timer = require "lua.timer"
local Sche = require "lua.sche"
local Socket = require "lua.socket"
local NameService = require "examples.rpc.toname"

local name_ip = "127.0.0.1"
local name_port = 8080

local count = 0

NameService.Init(name_ip,name_port)

local rpcserver = App.New()
rpcserver:RPCService("Plus",function (a,b)
	count = count + 1 
	return a+b 
end)

if not NameService.Register("Plus","127.0.0.1",8000)  then
	print("register to name error")	
	Exit()	
end

if not TcpServer.Listen("127.0.0.1",8000,function (client)
			print("on new client")		
			rpcserver:Add(client:Establish(Socket.Stream.RDecoder(),65535))		
	end) then
	print("server start on 127.0.0.1:8000")
	local last = C.GetSysTick()
	local timer = Timer.New():Register(function ()
		local now = C.GetSysTick()
		print(string.format("cocount:%d,rpccount:%f,elapse:%d",Sche.GetCoCount(),count*1000/(now-last),now-last))
		count = 0
		last = now
	end,1000):Run()
else
	print("server start error")
end

