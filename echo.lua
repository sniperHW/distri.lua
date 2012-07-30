--local registernet = assert(package.loadlib("./luanet.so","RegisterNet"))  
--registernet()

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

function mainloop()
	local lasttick = GetSysTick()
	local totalsend = 0;
	local netengine = CreateNet("127.0.0.1",8010)
	local connection_set = {}
	while true do
		local type,connection,rpacket = PeekMsg(netengine,50)
		if type then
			if type == 1 then
				table.insert(connection_set,connection)
				print("a connection comming")
			elseif type == 3 then
				
				for k,v in pairs(connection_set) do
					local wpkt = CreateWpacket(rpacket,0)
					SendPacket(connection,wpkt)
					totalsend = totalsend + 1
				end
				ReleaseRpacket(rpacket)

				--active close the connection
				--ActiveCloseConnection(connection)
			elseif type == 2 then
				print("disconnect")
				remove_connection(connection_set,connection)
				ReleaseConnection(connection)
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

--mainloop()  
