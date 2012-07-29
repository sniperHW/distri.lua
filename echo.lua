local registernet = assert(package.loadlib("./luanet.so","RegisterNet"))  
registernet()
function mainloop()
	local netengine = CreateNet("127.0.0.1",8010)
	while true do
		local type,connection,rpacket = PeekMsg(netengine,50)
		if type then
			if type == 1 then
				print("a connection comming")
			elseif type == 3 then
				local msg = PacketReadString(rpacket)
				print(msg)
			end
		end
		
	end	
end	

mainloop()  
