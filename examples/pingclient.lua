local socket = require "lua.socket"
local sche = require "lua.sche"
local App = require "lua.application"


local pingpong = App.New()

pingpong:Run(function ()
	for i=1,100 do
		sche.Spawn(function () 
			local client = socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
			if client:Connect("127.0.0.1",8000) then
				print("connect to 127.0.0.1:8000 error")
				return
			end
			client:Establish(CSocket.rpkdecoder())
			local wpk = CPacket.NewWPacket(64)
			wpk:Write_string("hello")
			client:Send(wpk) -- send the first packet
			pingpong:Add(client,function (s,rpk)
									s:Send(CPacket.NewWPacket(rpk))		
						        end)
		end)	
	end
end)
