--二进制封包echo
local sche = require "lua/sche"
local Packet = require "lua/packet"
local TcpServer = require "lua/tcpserver"
local Connection = require "lua/connection"
local App = require "lua/application"
local count = 0
local pingpong = App.New()
local function on_packet(conn,rpk)
	count = count + 1
	conn:Send(rpk)	
end

local function on_disconnected(conn,err)
	print("conn disconnected:" .. err)
	conn:Close()	
end

pingpong:Run(function ()
	TcpServer.Listen("127.0.0.1",8000,function (client)
		local  conn = Connection.New(client,Packet.RPacketDecoder.New(1024))		
		pingpong:Add(conn,on_packet,on_disconnected)		
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

