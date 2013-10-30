为lua提供简单的网络访问接口


sample echoserver

local registernet = assert(package.loadlib("./luanet.so","RegisterNet"))
registernet()
dofile("net/net.lua") 

function process_packet(connection,packet)
	send2all(connection_set,packet)
end

client_count = 0;


function _timeout(connection)
	active_close(connection)
end

function client_come(connection)
	client_count = client_count + 1
end

function client_go(connection)
	client_count = client_count - 1
end

function mainloop()
	local lasttick = GetSysTick()
	local n = net:new(process_packet,client_go):listen(client_come,arg[1],arg[2])
	net._send_timeout = _timeout
	net._recv_timeout = _timeout
	while n:run(50) == 0 do
		local tick = GetSysTick()
		if tick - 1000 >= lasttick then
			print("client:" .. client_count)
			lasttick = tick
		end
	end
	n = nil
	print("main loop end")
end	

mainloop()
