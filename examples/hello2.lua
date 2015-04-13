--原始数据echo
local Socket = require "lua.socket"
local Sche = require "lua.sche"
local Timer = require "lua.timer"


local count = 0
local server = Socket.Stream.New(CSocket.AF_INET)
if not server:Listen("127.0.0.1",8001) then
		print("hello2 listen on 127.0.0.1 8001")
		Sche.Spawn(function ()
						local last = C.GetSysTick()
						local timer = Timer.New():Register(function ()
							local now = C.GetSysTick()
							print("count:" .. count*1000/(now-last) .. " " .. now-last)
							count = 0
							last = now
						end,1000):Run()
		end)
		while true do
			local client = server:Accept()
			client:Establish()
			print("new client")
			Sche.Spawn(function ()
				while true do
					local packet,err = client:Recv(1000)
					if err then
						print("client disconnected err:" .. err)			
						client:Close()
						return
					end
					count = count + 1			
					client:Send(packet)
				end
			end)
		end		
else
	print("create server on 127.0.0.1 8001 error")
end

