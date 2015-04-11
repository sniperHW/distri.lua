local Socket = require "lua.socket"

local s = Socket.Datagram.New(CSocket.AF_INET,1024,Socket.Datagram.RDecoder())
for i = 1,3 do
	local wpk = Socket.WPacket(1024)
	wpk:Write_string("hello")
	s:Send(wpk,{CSocket.AF_INET,"127.0.0.1",8010})
end

while true do
	local rpk,from = s:Recv()
	s:Send(Socket.WPacket(rpk),from)
end