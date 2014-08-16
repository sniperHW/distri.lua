local socket = require "lua/socket"
local sche = require "lua/sche"
local Packet = require "lua/packet"

for i=1,100 do
	sche.Spawn(function () 
		local client = socket.new(luasocket.AF_INET,luasocket.SOCK_STREAM,luasocket.IPPROTO_TCP)
		if client:connect("127.0.0.1",8000) then
			print("connect to 127.0.0.1:8000 error")
			return
		end
		
		local wpk = Packet.WPacket(64)
		wpk:write_string("hello")
		client:send(wpk.bytebuffer)
		local decoder = Packet.RPacketDecoder(65535)
		print("connect success")
		while true do
			local data,err = client:recv()
			if err then
				print("client disconnected err:" .. err)			
				client:close()
				return
			end
			if decoder:putdata(data) then
				print("decoder:put error")
				client:close()
				return;
			end
			
			while true do
				local rpacket,err = decoder:unpack()
				if err then
					print("unpack error:" .. err)
					client:close()
					return
				end
				if not rpacket then
					break
				end
				client:send(rpacket.bytebuffer)			
			end
		end
	end)	
end

while true do
	sche.Sleep(100)
end
