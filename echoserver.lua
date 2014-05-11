local Socket = require "lua/socket"
local Tcp = require "lua/tcp"

function on_data(s,data,err)
	if not data then
		--print("a client disconnected")
		Socket.Close(s)
	else
		Tcp.Send(s,data)
		Socket.Close(s)
	end
end

function on_newclient(s)
	--print("on_newclient")
	if not Socket.Bind(s,on_data)then
		print("bind error")
		Socket.Close(s)
	end
end

print("server listen on 127.0.0.1 8010")
Tcp.Listen4({ip="127.0.0.1",port=8010},on_newclient)




