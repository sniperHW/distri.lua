--[[
测试用，已废弃
local net = require "lua/net/net"
local tcp = require "lua/net/tcp"

function on_data(s,data,err)
	if not data then
		print("a client disconnected")
		close(s)
	else
		tcp.send(s,data)
	end
end

function on_newclient(s)
	print("on_newclient")
	if not net.bind(s,on_data) then
		print("bind error")
		close(s)
	end
end

tcp.tcp_listen(net.netaddr_ipv4("127.0.0.1",8010),on_newclient)
]]--
--run()

