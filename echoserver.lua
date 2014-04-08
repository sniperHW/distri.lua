local net = require "lua/net/netaddr"
local tcp = require "lua/net/tcp"

function on_data(co,l,data,err)
	if not data then
		print("a client disconnected")
		close(l)
	else
		print(data)
	end
end

function on_newclient(co,l)
	print("on_newclient")
	print(l)
	if not net.bind(l,on_data) then
		print("bind error")
		close(l)
	end
end

tcp.tcp_listen(net.netaddr_ipv4("127.0.0.1",8010),on_newclient)

run()

