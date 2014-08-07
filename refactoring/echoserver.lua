local Socket = require "lua/socket"


local socket = Socket.new(luasocket.AF_INET,luasocket.SOCK_STREAM,luasocket.IPPROTO_TCP)

local function on_packet(client,packet)
	client:send(packet)
end

local function on_disconnected(client,err)
	print("client disconnected")
end

local function on_new_client(client)
	client:set_packet_callback(on_packet)
	client:set_disconn_callback(on_disconnected)
	client:establish(4096)
end

print(luasocket.IPPROTO_TCP)

local err = socket:listen("127.0.0.1",8010,on_new_client) 
if not err then
	print("server listen on 127.0.0.1 8010")
end
