local Socket = require "lua.socket"
local Timer = require "lua.timer"
local Sche = require "lua.sche"

local count = 0
local s = Socket.Datagram.New(CSocket.AF_INET,1024,Socket.Datagram.RDecoder())
s:Listen("127.0.0.1",8010)


Sche.Spawn(function ()
	while true do
		local rpk,from = s:Recv()
		--print(from[1],from[2],from[3])
		count = count + 1
		s:Send(Socket.WPacket(rpk),from)
	end
end)

local last = C.GetSysTick()
local timer = Timer.New():Register(function ()
	local now = C.GetSysTick()
	print(string.format("count:%f",count*1000/(now-last)))
	count = 0
	last = now
end,1000):Run()

