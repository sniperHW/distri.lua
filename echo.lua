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
		local wpkt = CreateWpacket(rpacket,0)
		SendPacket(v,wpkt)
		totalsend = totalsend + 1
	end
end

function mainloop()
	local lasttick = GetSysTick()
	local netengine = CreateNet("127.0.0.1",8011)
	local connection_set = {}
	local stop = 0
	while stop == 0 do
		
		local msgs = PeekMsg(netengine,50)
		if msgs ~= nil then
			for k,v in pairs(msgs) do
				if v[1] == 1 then
					table.insert(connection_set,v[2])
					print("a connection comming")
				elseif v[1] == 3 then
					
					send2all(connection_set,v[3])
					ReleaseRpacket(v[3])
				elseif v[1] == 2 then
					if 1 == ReleaseConnection(v[2]) then
						remove_connection(connection_set,v[2])
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
			print(totalsend)
			totalsend = 0;
			lasttick = tick
		end
		
	end	
	print("main loop end")
end	

mainloop()  
