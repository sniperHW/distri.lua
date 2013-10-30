local registernet = assert(package.loadlib("./luanet.so","RegisterNet"))  
registernet()
dofile("net/net.lua")

local ava_delay = 0
local last_print = 0
local total_recv = 0;
local connection_set = {}

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
	total_recv = total_recv + 1
end

function clientsSend()
	for k,v in pairs(connection_set) do
		local wpkt = wpacket:net()
		wpkt:write_number(GetSysTick())
		wpkt:write_string("hello kenny")
		SendPacket(v,wpkt)				
	end
end

function mainloop()
	local n = net:new(process_packet,remove_connection)
	net._send_timeout = _timeout
	net._recv_timeout = _timeout
	for i=1,arg[3] do
		n:connect(add_connection,arg[1],arg[2],500)
	end
	while n:run(50) == 0 do
		local nowtick = GetSysTick()
		if nowtick >= last_print+1000 then
			print("ava_delay:" .. ava_delay)
			print("total recv:" .. total_recv)
			ava_delay = 0
			total_recv = 0
			last_print = nowtick
		end
		clientsSend()
	end
	n = nil
	print("main loop end")
end	
mainloop() 
