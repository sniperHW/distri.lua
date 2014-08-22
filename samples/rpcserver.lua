local TcpServer = require "lua/tcpserver"
local App = require "lua/application"
local RPC = require "lua/rpc"
local Timer = require "lua/timer"

local count = 0

local function on_disconnected(conn,err)
	print("conn disconnected:" .. err)	
end

local rpcserver = App.New()

rpcserver:RPCService("Plus",function (a,b)
	count = count + 1 
	return a+b 
end)

rpcserver:Run(function ()
	TcpServer.Listen("127.0.0.1",8000,65535,CSocket.rpkdecoder(),function (client)
			print("on new client")
			rpcserver:Add(client,nil,on_disconnected)		
	end)
end)

print("server start on 127.0.0.1:8000")


local last = GetSysTick()
local timer = Timer.New():Register(function ()
	local now = GetSysTick()
	print("count:" .. count*1000/(now-last) .. " " .. now-last)
	count = 0
	last = now
	return true --return true to register once again	
end,1000):Run()


