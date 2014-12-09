--二进制封包echo
local sche = require "lua.sche"
local TcpServer = require "lua.tcpserver"
local App = require "lua.application"
local Timer = require "lua.timer"
local Time = require "lua.time"

local count = 0
local pingpong = App.New()
local size = 0
local function on_packet(s,rpk)
	count = count + 1
	size = size + 10240
	s:Send(CPacket.NewWPacket(rpk))	
end

local success

pingpong:Run(function ()
	success = not TcpServer.Listen("127.0.0.1",8000,function (client)
		client:Establish(CSocket.rpkdecoder(65535))
		pingpong:Add(client,on_packet,function () print("disconnected") end,5000)		
	end)
end)

if success then
	print("hello start on 127.0.0.1:8000")
	local last = Time.SysTick()
	local timer = Timer.New():Register(function ()
		local now = Time.SysTick()
		--print("count:" .. count*1000/(now-last) .. " " .. now-last)
		print(string.format("count:%d,size:%d MB",count*1000/(now-last),size*1000/(now-last)/1024/1024))
		count = 0
		size = 0
		last = now
		return true --return true to register once again	
	end,1000):Run()
else
	print("hello start error\n")
end

