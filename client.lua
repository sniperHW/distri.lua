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
	for i=1,arg[3] do
		AsynConnect(netengine,arg[1],arg[2],0)
	end
	local stop = 0
	while stop == 0 do
		local msgs = PeekMsg(netengine,50)
		if msgs ~= nil then
			for k,v in pairs(msgs) do
				local type,con,rpk = v[1],v[2],v[3]
				if type == NEW_CONNECTION then
					print("a connection comming")
				elseif type == PROCESS_PACKET then
					local handle = PacketReadNumber(rpk)
					local selfhandle = GetHandle(con)
					if handle == selfhandle then
						local tick = PacketReadNumber(rpk)
						tick = GetSysTick() - tick
						ava_delay = (ava_delay + tick)/2
					end
					total_recv = total_recv + 1
					ReleaseRpacket(rpk)
				elseif type == DISCONNECT then
					remove_connection(connection_set,con)
					if 1 == ReleaseConnection(con) then
						print("disconnect")
					end	
				elseif type == CONNECT_SUCESSFUL then
					print("connect success")
					table.insert(connection_set,con)	
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
