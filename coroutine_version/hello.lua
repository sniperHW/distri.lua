local socket = require "lua/socket"
local sche = require "lua/sche"

local server = socket.new(luasocket.AF_INET,luasocket.SOCK_STREAM,luasocket.IPPROTO_TCP)
if not server:listen("127.0.0.1",8091) then
		print("server listen on 127.0.0.1 8091")
else
	print("create server on 127.0.0.1 8091 error")
	return
end

while true do
	local client = server:accept()
	print("new client")
	sche.Spawn(function ()
		while true do
			local packet,err = client:recv(10)
			if err then
				print("client disconnected err:" .. err)			
				client:close()
				return
			end			
			client:send(packet)
		end
	end)
end


