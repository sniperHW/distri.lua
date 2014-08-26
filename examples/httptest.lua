local Socket = require "lua/socket"
local Sche = require "lua/sche"

local count = 0
local server = Socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
if not server:Listen("127.0.0.1",8001) then
		print("hello2 listen on 127.0.0.1 8001")
		while true do
			local client = server:Accept()
			client:Establish(CHttp.http_decoder(CHttp.HTTP_REQUEST,65535))
			Sche.Spawn(function ()
				print("client")
				while true do
					local packet,err = client:Recv()
					if err then
						print("client disconnected err:" .. err)			
						client:Close()
						return
					end
					local event_type = packet:ToString()
					print(event_type)
					if event_type == "ON_MESSAGE_COMPLETE" then
						local wpk = CPacket.NewRawPacket("HTTP/1.0 200 OK\r\nContent-type: text/plain\r\nContent-length: 19\r\n\r\nHi! I'm a message!")
						client:Send(wpk)
						client:Close()
						return
					end
				end
			end)
		end		
else
	print("create server on 127.0.0.1 8001 error")
end
