local socket = require "lua/socket"
local sche = require "lua/sche"
local Packet = require "lua/packet"
local Connection = require "lua/connection"
local App = require "lua/application"


local pingpong = App.New()

local function on_packet(conn,rpk)
	conn:Send(rpk)	
end

local function on_disconnected(conn,err)
	print("conn disconnected:" .. err)
	conn:Close()	
end

pingpong:Run(function ()
	for i=1,100 do
		sche.Spawn(function () 
			local client = socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
			if client:Connect("127.0.0.1",8000) then
				print("connect to 127.0.0.1:8000 error")
				return
			end
			local conn = Connection.New(client,Packet.RPacketDecoder.New(1024))
			local wpk = Packet.WPacket.New(64)
			wpk:Write_string("hello")
			conn:Send(wpk) -- send the first packet
			pingpong:Add(conn,on_packet,on_disconnected)
		end)	
	end
end)

while true do
	sche.Sleep(100)
end
