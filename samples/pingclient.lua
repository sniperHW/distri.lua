local socket = require "lua/socket"
local sche = require "lua/sche"
local App = require "lua/application"


local pingpong = App.New()

local function on_packet(s,rpk)
	s:Send(CPacket.NewWPacket(rpk))		
end

local function on_disconnected(conn,err)
	print("conn disconnected:" .. err)
	conn:Close()	
end

pingpong:Run(function ()
	for i=1,100 do
		sche.Spawn(function () 
			local client = socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
			if client:Connect("127.0.0.1",8000,65535,CSocket.rpkdecoder()) then
				print("connect to 127.0.0.1:8000 error")
				return
			end
			local wpk = CPacket.NewWPacket(64)
			wpk:Write_string("hello")
			client:Send(wpk) -- send the first packet
			pingpong:Add(client,on_packet,on_disconnected)
		end)	
	end
end)

while true do
	sche.Sleep(100)
end
