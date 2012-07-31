local registernet = assert(package.loadlib("./luanet.so","RegisterNet"))  
registernet()
function mainloop()
	local netengine = CreateNet(nil,0)
	print("engine create successful")
	for i=1,200 do
		AsynConnect(netengine,"127.0.0.1",8011,0)
	end
	while true do
		local type,connection,rpacket = PeekMsg(netengine,50)
		if type then
			if type == 1 then
				print("a connection comming")
			elseif type == 3 then
				--local selfhandle = GetHandle(connection)
				local handle = PacketReadNumber(rpacket)
				--if handle == selfhandle then
					local wpkt = CreateWpacket(nil,64)
					PacketWriteNumber(wpkt,handle)
					PacketWriteString(wpkt,"hello kenny")
					SendPacket(connection,wpkt)
				--end
				ReleaseRpacket(rpacket)
				--active close the connection
				--ActiveCloseConnection(connection)
			elseif type == 2 then
				print("disconnect")
				ReleaseConnection(connection)
			elseif type == 4 then
				print("connect success")
				local wpkt = CreateWpacket(nil,64)
				local handle = GetHandle(connection)
				PacketWriteNumber(wpkt,handle)
				PacketWriteString(wpkt,"hello kenny")
				SendPacket(connection,wpkt)
			else
				print("break mainloop")
				break
			end
		end
		
	end	
	print("main loop end")
end	
mainloop() 
