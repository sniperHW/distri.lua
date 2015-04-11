local socket = require "lua.socket"

local s = socket.Datagram(CSocket.AF_INET,1024,CSocket.datagram_rpkdecoder())
for i = 1,3 do
	local wpk = CPacket.NewWPacket(1024)
	wpk:Write_string("hello")
	s:Send(wpk,{CSocket.AF_INET,"127.0.0.1",8010})
end

while true do
	local rpk,from = s:Recv()
	--print("recv")
	s:Send(CPacket.NewWPacket(rpk),from)
end