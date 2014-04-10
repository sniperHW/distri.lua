--[[
测试用，已废弃
local net = require "lua/net/net"
local tcp = require "lua/net/tcp"

function on_data(l,data,err)
	if not data then
		print("a client disconnected")
		close(l)
	else
		tcp.send(l,data)
	end
end

function on_connected(s,remote_addr,err)
	print("on_connected")
	if s then
		if not net.bind(s,on_data) then
			print("bind error")
			close(s)
		else
			tcp.send(s,"hahaha")	
		end
	end	
end

for i=1,10 do
	tcp.tcp_asynconnect(net.netaddr_ipv4("127.0.0.1",8010),nil,on_connected,3000)
end
]]--
--run()
