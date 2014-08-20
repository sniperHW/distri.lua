local socket = require "lua/socket"
local sche = require "lua/sche"
local Packet = require "lua/packet"
local Connection = require "lua/connection"
local App = require "lua/application"


local pingpong = App.Application()

local function on_packet(conn,rpk)
	conn:send(rpk)	
end

local function on_disconnected(conn,err)
	print("conn disconnected:" .. err)
	conn:close()	
end

pingpong:run(function ()
	for i=1,10 do
		sche.Spawn(function () 
			local client = socket.new(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
			if client:connect("127.0.0.1",8000) then
				print("connect to 127.0.0.1:8000 error")
				return
			end
			local conn = Connection.Connection(client,Packet.RPacketDecoder(1024))
			local wpk = Packet.WPacket(64)
			wpk:write_string("hello")
			conn:send(wpk) -- send the first packet
			pingpong:add(conn,on_packet,on_disconnected)
		end)	
	end
end)

while true do
	sche.Sleep(100)
end
