local sche = require "lua.sche"
local TcpServer = require "lua.tcpserver"
local App = require "lua.application"
local Timer = require "lua.timer"
local Socket = require "lua.socket"


local clients = {}
local clientcount = 0
local count = 0
local pingpong = App.New()
local size = 0
local function on_packet(s,rpk)
	for k,v in pairs(clients) do
		count = count + 1
		v:Send(Socket.WPacket(rpk))
	end	
end

local success = not TcpServer.Listen("127.0.0.1",8000,function (client)
		client:Establish(Socket.Stream.RDecoder(1024),1024)
		clients[client] = client
		clientcount = clientcount + 1
		pingpong:Add(client,on_packet,function () 
						 clients[client] = nil 
					             	 clientcount = clientcount - 1 
					             end)		
	end)

if success then
	print("hello start on 127.0.0.1:8000")
	local last = C.GetSysTick()
	local timer = Timer.New():Register(function ()
		local now = C.GetSysTick()
		print(string.format("clientcount:%d,count:%f",clientcount,count*1000/(now-last)))
		count = 0
		size = 0
		last = now
	end,1000):Run()
else
	print("hello start error\n")
end