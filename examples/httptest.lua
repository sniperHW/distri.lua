local Socket = require "lua/socket"
local Sche = require "lua/sche"
local response = "HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-Type: text/plain\r\nDate: Tue, 26 Aug 2014 11:40:38 GMT\r\nTransfer-Encoding: chunked\r\n\r\nc\r\nHello World\n\r\n0\r\n\r\n"
local count = 0
local server = Socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
if not server:Listen("127.0.0.1",8001) then
		print("hello2 listen on 127.0.0.1 8001")
		while true do
			local client = server:Accept()
			client:Establish(CHttp.http_decoder(CHttp.HTTP_REQUEST,512),65535)
			Sche.Spawn(function ()
				while true do
					local packet,err = client:Recv()
					if err then		
						client:Close()
						return					
					elseif packet:ToString() == "ON_MESSAGE_COMPLETE" then
						local wpk = CPacket.NewRawPacket(response)
						client:Send(wpk)
					end
				end
			end)
		end		
else
	print("create server on 127.0.0.1 8001 error")
end
