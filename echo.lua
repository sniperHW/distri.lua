local registernet = assert(package.loadlib("./luanet.so","RegisterNet"))  
registernet()
function mainloop()
	local netengine = CreateNet("127.0.0.1",8012)
	while true do
		local type,connection,rpacket = PeekMsg(netengine,50)
		if type then
			if type == 1 then
				print("a connection comming")
			elseif type == 3 then
				--local wpkt = CreateWpacket(rpacket,0) 
				--SendPacket(connection,wpkt)
				local msg = PacketReadString(rpacket)
				print(msg)
				ReleaseRpacket(rpacket)
				ActiveCloseConnection(connection)
			elseif type == 2 then
				print("disconnect")
				ReleaseConnection(connection)
			else
				break
			end
		end
		
	end	
	print("main loop end")
end	

mainloop()  
