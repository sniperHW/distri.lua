local registernet = assert(package.loadlib("./luanet.so","RegisterNet"))
registernet()
dofile("net/net.lua") 
connection_set = {}
function remove_connection(connection)
	local idx = 0
	for k,v in pairs(connection_set) do
		if v == connection then
			idx = k
		end
	end
	if idx >= 1 then
		table.remove(connection_set,idx)
	end
end

function add_connection(connection)
	table.insert(connection_set,connection)
end

function process_packet(connection,packet)
	send2all(connection_set,packet)
end

local totalsend = 0
function send2all(packet)
	for k,v in pairs(connection_set) do
		SendPacket(v,packet)
		totalsend = totalsend + 1
	end
end

function _timeout(connection)
	active_close(connection)
end

function mainloop()
	local lasttick = GetSysTick()
	local n = net:new(process_packet,remove_connection):listen(add_connection,arg[1],arg[2])
	net._send_timeout = _timeout
	net._recv_timeout = _timeout
	while n:run(50) == 0 do
		local tick = GetSysTick()
		if tick - 1000 >= lasttick then
			print("client:" .. #connection_set .. " packet send:" ..totalsend)
			totalsend = 0;
			lasttick = tick
		end
	end
	n = nil
	print("main loop end")
end	

mainloop()
