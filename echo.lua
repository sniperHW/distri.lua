local registernet = assert(package.loadlib("./luanet.so","RegisterNet"))  
registernet()

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
local totalsend = 0
function send2all(connection_set,rpacket)
	for k,v in pairs(connection_set) do
		local wpkt = CreateWpacket(v,rpacket,0)
		SendPacket(v,wpkt,0)
		totalsend = totalsend + 1
	end
end

function mainloop()
	local lasttick = GetSysTick()
	local netengine = CreateNet(arg[1],arg[2],0)
	local connection_set = {}
	local stop = 0
	while stop == 0 do
		
		local msgs = PeekMsg(netengine,50)
		if msgs ~= nil then
			for k,v in pairs(msgs) do
				local type,con,rpk = v[1],v[2],v[3]
				if type == NEW_CONNECTION then
					table.insert(connection_set,con)
					print("a connection comming")
				elseif type == PROCESS_PACKET then
					send2all(connection_set,rpk)
					ReleaseRpacket(rpk)
				elseif type == DISCONNECT then
					if 1 == ReleaseConnection(con) then
						remove_connection(connection_set,con)
						print("disconnect")
					end
				else
					print("break main loop")
					stop = 1
					break
				end
				
			end 
		end
		local tick = GetSysTick()
		if tick - 1000 >= lasttick then
			print("client:" .. #connection_set .. " packet send:" ..totalsend)
			totalsend = 0;
			lasttick = tick
		end
		
	end
	DestroyNet(netengine)	
	print("main loop end")
end	

mainloop()  
