local sche = require "lua/sche"
local Packet = require "lua/packet"
local TcpServer = require "lua/tcpserver"
local Connection = require "lua/connection"

local count = 0

TcpServer.Listen("127.0.0.1",8000,function (client)
		local  conn = Connection.Connection(client,Packet.RPacketDecoder(65535))
		while true do
			local rpk,err = conn:recv()
			if err then
				print("conn disconnected:" .. err)
				conn:close()
				return
			end
			count = count + 1
			conn:send(rpk)	
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

