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

function mainloop()
	local lasttick = GetSysTick()
	local netengine = CreateNet(arg[1],arg[2],1)
	local connection_set = {}
	local last_check_timeout = GetSysTick()
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
					print("--------------------------")
					print(PacketReadString(rpk))
					local wpkt = CreateWpacket(con,0,128)
					PacketWriteString(wpkt,"<html><body>hello kenny</body></html>\n")
					SendPacket(con,wpkt,1)
					ReleaseRpacket(rpk)
				elseif type == DISCONNECT then
					if 1 == ReleaseConnection(con) then
						remove_connection(connection_set,con)
						print("disconnect")
					else
						print("release failed")
					end
				elseif type == PACKET_SEND_FINISH then
					ActiveCloseConnection(con)
				elseif type == CONNECTION_TIMEOUT then
					ActiveCloseConnection(con)	
				else
					print("break main loop")
					stop = 1
					break
				end
				
			end 
		end
		
		local tick = GetSysTick()
		--[[if tick - 1000 >= lasttick then
			print("client:" .. #connection_set .. " packet send:" ..totalsend)
			totalsend = 0;
			lasttick = tick
		end]]--
		
	end
	DestroyNet(netengine)	
	print("main loop end")
end	

mainloop()  
