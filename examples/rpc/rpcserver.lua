local TcpServer = require "lua.tcpserver"
local App = require "lua.application"
local RPC = require "lua.rpc"
local Timer = require "lua.timer"
local Sche = require "lua.sche"
local Socket = require "lua.socket"


local name_ip = "127.0.0.1"
local name_port = 8080

local count = 0

local rpcserver = App.New()

local toname = Socket.Stream.New(CSocket.AF_INET)
if toname:Connect(name_ip,name_port) then
	print("connect to name error")
	Exit()
end

rpcserver:Add(toname:Establish(Socket.Stream.RDecoder(1024),1024))

local rpcHandler = RPC.MakeRPC(toname,"Register")
local err = rpcHandler:CallSync("Plus","127.0.0.1",8000)
if err then
	print("register to name error")	
	Exit()
end

rpcserver:RPCService("Plus",function (a,b)
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
		print(string.format("cocount:%d,rpccount:%f,elapse:%d",Sche.GetCoCount(),count*1000/(now-last),now-last))
		count = 0
		last = now
	end,1000):Run()
else
	print("server start error")
end

