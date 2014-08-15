local Socket = require "lua/socket"

local function on_packet(client,packet)
	client:send(packet)
end

local function on_disconnected(client,err)
	print("client disconnected")
end

if not Socket.new(luasocket.AF_INET,
				  luasocket.SOCK_STREAM,
				  luasocket.IPPROTO_TCP):listen("127.0.0.1",8010,function (client)
	client:set_packet_callback(on_packet)
	client:set_disconn_callback(on_disconnected)
	client:establish(4096)
end) then
	print("server listen on 127.0.0.1 8010")
end



