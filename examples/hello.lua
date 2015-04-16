--二进制封包echo
local sche = require "lua.sche"
local TcpServer = require "lua.tcpserver"
local App = require "lua.application"
local Timer = require "lua.timer"
local Socket = require "lua.socket"


local count = 0
local pingpong = App.New()
local size = 0
local function on_packet(s,rpk)
	count = count + 1
	size = size + 10240
	s:Send(CPacket.NewWPacket(rpk))	
end

local success

--pingpong:Run(function ()
local success = not TcpServer.Listen("127.0.0.1",8000,function (client)
		client:Establish(Socket.Stream.RDecoder(65535))
		pingpong:Add(client,on_packet,function () print("disconnected") end,5000)		
	end)
--end)

if success then
	print("hello start on 127.0.0.1:8000")
	local last = C.GetSysTick()
	local timer = Timer.New():Register(function ()
		local now = C.GetSysTick()
		--print("count:" .. count*1000/(now-last) .. " " .. now-last)
		print(string.format("count:%d,size:%f MB",count*1000/(now-last),size*1000/(now-last)/1024/1024))
		count = 0
		size = 0
		last = now
	end,1000):Run()
else
	print("hello start error\n")
end

