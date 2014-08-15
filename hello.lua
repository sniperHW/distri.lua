local socket = require "lua/socket"
local sche = require "lua/sche"

local server = socket.new(luasocket.AF_INET,luasocket.SOCK_STREAM,luasocket.IPPROTO_TCP)
if not server:listen("127.0.0.1",8091) then
		print("server listen on 127.0.0.1 8091")
else
	print("create server on 127.0.0.1 8091 error")
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
	print("new client")
	sche.Spawn(function ()
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


