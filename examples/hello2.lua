--原始数据echo
local Socket = require "lua.socket"
local Sche = require "lua.sche"
local Timer = require "lua.timer"
local Time = require "lua.time"

local count = 0
local server = Socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
if not server:Listen("127.0.0.1",8001) then
		print("hello2 listen on 127.0.0.1 8001")
		Sche.Spawn(function ()
						local last = Time.SysTick()
						local timer = Timer.New():Register(function ()
							local now = Time.SysTick()
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
					local packet,err = client:Recv()
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

