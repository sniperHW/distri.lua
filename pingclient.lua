local socket = require "lua/socket"
local sche = require "lua/sche"
local Packet = require "lua/packet"
local Connection = require "lua/connection"

for i=1,1000 do
	sche.Spawn(function () 
		local client = socket.new(luasocket.AF_INET,luasocket.SOCK_STREAM,luasocket.IPPROTO_TCP)
		if client:connect("127.0.0.1",8000) then
			print("connect to 127.0.0.1:8000 error")
			return
		end
		local conn = Connection.Connection(client,Packet.RPacketDecoder(65535))
		local wpk = Packet.WPacket(64)
		wpk:write_string("hello")
		conn:send(wpk) -- send the first packet
		while true do
			local rpk,err = conn:recv()
			if err then
				print("conn disconnected:" .. err)
				conn:close()
				return
			end
			conn:send(rpk)	
		end
	end)	
end

while true do
	sche.Sleep(100)
end
