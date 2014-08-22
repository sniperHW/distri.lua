--二进制封包echo
local sche = require "lua/sche"
local TcpServer = require "lua/tcpserver"
local App = require "lua/application"
local count = 0
local pingpong = App.New()
local function on_packet(s,rpk)
	count = count + 1
	s:Send(CPacket.NewWPacket(rpk))	
end

local function on_disconnected(s,err)
	print("conn disconnected:" .. err)
	s:Close()	
end

pingpong:Run(function ()
	TcpServer.Listen("127.0.0.1",8000,function (client)
		client:Establish(CSocket.rpkdecoder())
		pingpong:Add(client,on_packet,on_disconnected)		
	end)
end)

print("server start on 127.0.0.1:8000")

local tick = GetSysTick()
local now = GetSysTick()
while true do 
	now = GetSysTick()
	if now - tick >= 1000 then
		print("count:" .. count*1000/(now-tick) .. " " .. now-tick)		
		tick = now
		count = 0
	end
	sche.Sleep(10)
end

