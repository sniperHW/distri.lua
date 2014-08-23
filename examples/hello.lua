--二进制封包echo
local sche = require "lua/sche"
local TcpServer = require "lua/tcpserver"
local App = require "lua/application"
local Timer = require "lua/timer"

local count = 0
local pingpong = App.New()
local function on_packet(s,rpk)
	count = count + 1
	s:Send(CPacket.NewWPacket(rpk))	
end

local success

pingpong:Run(function ()
	success = not TcpServer.Listen("127.0.0.1",8000,function (client)
		client:Establish(CSocket.rpkdecoder())
		pingpong:Add(client,on_packet)		
	end)
end)

if success then
	print("server start on 127.0.0.1:8000")
	local last = GetSysTick()
	local timer = Timer.New():Register(function ()
		local now = GetSysTick()
		print("count:" .. count*1000/(now-last) .. " " .. now-last)
		count = 0
		last = now
		return true --return true to register once again	
	end,1000):Run()
else
	print("server start error\n")
end

