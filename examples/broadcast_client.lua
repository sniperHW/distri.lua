local socket = require "lua.socket"
local sche = require "lua.sche"
local App = require "lua.application"
local Timer = require "lua.timer"

local pingpong = App.New()
local delay = 0

local function on_packet(s,rpk)
     local id = rpk:Read_uint32()
     --print(id)
     if id == s.id then
            local tick = rpk:Read_uint32()
            local wpk = CPacket.NewWPacket(64)
            wpk:Write_uint32(s.id)
            wpk:Write_uint32(C.GetSysTick())
            s:Send(wpk)
            local d = C.GetSysTick() - tick
            delay = (delay + d)/2
     end
end

for i=1,100 do
       sche.Spawn(function () 
		local client = socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
		if client:Connect("127.0.0.1",8000) then
			print("connect to 127.0.0.1:8000 error")
			return
		end
		client:Establish(CSocket.rpkdecoder(65535))
		local wpk = CPacket.NewWPacket(64)
	                     client.id = i
                                          wpk:Write_uint32(client.id)
		wpk:Write_uint32(C.GetSysTick())
                                          client:Send(wpk) -- send the first packet
		pingpong:Add(client,on_packet, function () print("disconnected")   end)
                              end)	
end
local last = C.GetSysTick()
local timer = Timer.New():Register(function ()
      local now = C.GetSysTick()
      print(string.format("avadelay:%d",delay))
      last = now
end,1000):Run()

