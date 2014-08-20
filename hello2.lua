--原始数据echo
local Socket = require "lua/socket"
local Sche = require "lua/sche"

local server = Socket.new(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
if not server:listen("127.0.0.1",8010) then
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
	local client = server:accept()
	print("new client")
	Sche.Spawn(function ()
		while true do
			local packet,err = client:recv()
			if err then
				print("client disconnected err:" .. err)			
				client:close()
				return
			end
			count = count + 1			
			client:send(packet)
		end
	end)
end
