local sche = require "lua/sche"
local Packet = require "lua/packet"
local TcpServer = require "lua/tcpserver"

local count = 0

TcpServer.Listen("127.0.0.1",8000,function (client)
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

print("server start on 127.0.0.1:8000")

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

