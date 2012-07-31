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
	else
		print("remove fail")
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
	while true do
		local type,connection,rpacket = PeekMsg(netengine,50)
		if type then
			if type == 1 then
				table.insert(connection_set,connection)
				print("a connection comming")
			elseif type == 3 then
				
				send2all(connection_set,rpacket)
				ReleaseRpacket(rpacket)
			elseif type == 2 then
				if 1 == ReleaseConnection(connection) then
					remove_connection(connection_set,connection)
					print("disconnect")
				end
			else
				print("break main loop")
				break
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
