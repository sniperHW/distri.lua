local registernet = assert(package.loadlib("./luanet.so","RegisterNet"))  
registernet()

local ava_delay = 0
local last_print = 0
local total_recv = 0;

function remove_connection(connection_set,connection)
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

function clientsSend(connection_set)
	for k,v in pairs(connection_set) do
		local wpkt = CreateWpacket(nil,64)
		local handle = GetHandle(v)
		PacketWriteNumber(wpkt,handle)
		PacketWriteNumber(wpkt,GetSysTick())
		PacketWriteString(wpkt,"hello kenny")
		SendPacket(v,wpkt)		
	end
end

function mainloop()
	local netengine = CreateNet(nil,0)
	local connection_set = {}
	print("engine create successful")
	for i=1,100 do
		AsynConnect(netengine,"127.0.0.1",8011,0)
	end
	local stop = 0
	while stop == 0 do
		local msgs = PeekMsg(netengine,50)
		if msgs ~= nil then
			for k,v in pairs(msgs) do
				if v[1] == 1 then
					print("a connection comming")
				elseif v[1] == 3 then
					local handle = PacketReadNumber(v[3])
					local selfhandle = GetHandle(v[2])
					if handle == selfhandle then
						local tick = PacketReadNumber(v[3])
						tick = GetSysTick() - tick
						ava_delay = (ava_delay + tick)/2
					end
					total_recv = total_recv + 1
					ReleaseRpacket(v[3])
				elseif v[1] == 2 then
					remove_connection(connection_set,v[2])
					if 1 == ReleaseConnection(v[2]) then
						print("disconnect")
					end	
				elseif v[1] == 4 then
					print("connect success")
					table.insert(connection_set,v[2])	
				else
					print("break mainloop")
					stop = 1
					break
				end			
			end
		end
		
		local nowtick = GetSysTick()
		if nowtick >= last_print+1000 then
			print("ava_delay:" .. ava_delay)
			print("total recv:" .. total_recv)
			ava_delay = 0
			total_recv = 0
			last_print = nowtick
		end
		clientsSend(connection_set)
	end	
	print("main loop end")
end	
mainloop() 
