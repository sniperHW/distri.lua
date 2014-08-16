local socket = require "lua/socket"
local sche = require "lua/sche"
local Packet = require "lua/packet"

local server = socket.new(luasocket.AF_INET,luasocket.SOCK_STREAM,luasocket.IPPROTO_TCP)
if not server:listen("127.0.0.1",8010) then
		print("server listen on 127.0.0.1 8010")
else
	print("create server on 127.0.0.1 8010 error")
	return
end

local count = 0

sche.Spawn(function ()
	local tick = GetSysTick()
	local now = GetSysTick()
	while true do 
		now = GetSysTick()
		if now - tick >= 1000 then
			print(count*1000/(now-tick) .. " " .. now-tick)
			tick = now
			count = 0
		end
		sche.Sleep(10)
	end
end)

while true do
	local client = server:accept()
	sche.Spawn(function ()	
		local decoder = Packet.RPacketDecoder(65535)
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
				count = count + 1
				client:send(rpacket.bytebuffer)			
			end
		end
	end)
end


