local registernet = assert(package.loadlib("./luanet.so","RegisterNet"))  
registernet()
dofile("net/net.lua")

local recv_count = 0;

function connect_ok(connection)
	if connection then
		SendPacket(connection,"hello kenny")
	else
		print("connect timeout")
	end
end

function process_packet(connection,packet)
	recv_count = recv_count + 1
	SendPacket(connection,"hello kenny")
end

function _timeout(connection)
	active_close(connection)
end

function mainloop()
	local n = net:new(process_packet)
	net._send_timeout = _timeout
	net._recv_timeout = _timeout
	for i=1,arg[3] do
		n:connect(add_connection,arg[1],arg[2],500)
	end
	local lasttick = GetSysTick()
	while n:run(50) == 0 do
		local tick = GetSysTick()
		if tick - 1000 >= lasttick then
			print("recv_count:" .. recv_count)
			lasttick = tick
		end
	end
	n = nil
	print("main loop end")
end	
mainloop() 
