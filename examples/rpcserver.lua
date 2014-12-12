local TcpServer = require "lua.tcpserver"
local App = require "lua.application"
local RPC = require "lua.rpc"
local Timer = require "lua.timer"
local Sche = require "lua.sche"


local count = 0

local rpcserver = App.New()

rpcserver:RPCService("Plus",function (_,a,b)
	count = count + 1 
	return a+b 
end)

local success

rpcserver:Run(function ()
	success = not TcpServer.Listen("127.0.0.1",8000,function (client)
			print("on new client")		
			rpcserver:Add(client:Establish(CSocket.rpkdecoder()))		
	end)
end)

if success then
	print("server start on 127.0.0.1:8000")
	local last = C.GetSysTick()
	local timer = Timer.New():Register(function ()
		local now = C.GetSysTick()
		print(string.format("cocount:%d,rpccount:%d,elapse:%d",Sche.GetCoCount(),count*1000/(now-last),now-last))
		count = 0
		last = now
	end,1000):Run()
else
	print("server start error")
end

