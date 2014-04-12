local net = require "lua/net"
local table2str = require "lua/table2str"

function on_data(s,data,err)
	if not data then
		print("a client disconnected")
		close(s)
	else
		local tb = table2str.Str2Table(data)
		_send(s,table2str.Table2Str(tb),nil)
	end
end

function on_newclient(s)
	print("on_newclient")
	if not _bind(s,{recvfinish = on_data})then
		print("bind error")
		close(s)
	end
end

listen(IPPROTO_TCP,SOCK_STREAM,net.netaddr_ipv4("127.0.0.1",8010),{onaccept=on_newclient})




