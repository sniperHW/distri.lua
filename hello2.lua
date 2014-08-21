--原始数据echo
local Socket = require "lua/socket"
local Sche = require "lua/sche"

local server = Socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
if not server:Listen("127.0.0.1",8010) then
		print("server listen on 127.0.0.1 8010")
else
	print("create server on 127.0.0.1 8010 error")
	return
end

local count = 0

Sche.Spawn(function ()
	local tick = GetSysTick()
	local now = GetSysTick()
	while true do 
		now = GetSysTick()
		if now - tick >= 1000 then
			print(count*1000/(now-tick) .. " " .. now-tick)
			tick = now
			count = 0
		end
		Sche.Sleep(10)
	end
end)

while true do
	local client = server:Accept()
	print("new client")
	Sche.Spawn(function ()
		while true do
			local packet,err = client:Recv()
			if err then
				print("client disconnected err:" .. err)			
				client:Close()
				return
			end
			count = count + 1			
			client:Send(packet)
		end
	end)
end
