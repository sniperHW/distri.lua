local Socket = require "lua/socket"
local Sche = require "lua/sche"
local Packet = require "lua/packet"
local Connection = require "lua/connection"
local App = require "lua/application"
local RPC = require "lua/rpc"

local rpcclient = App.Application()

local function on_disconnected(conn,err)
	print("conn disconnected:" .. err)	
end
local count = 0
rpcclient:run(function ()
	for i=1,1 do
		Sche.Spawn(function () 
			local client = Socket.new(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
			if client:connect("127.0.0.1",8000) then
				print("connect to 127.0.0.1:8000 error")
				return
			end
			local conn = Connection.Connection(client,Packet.RPacketDecoder(65535))
			rpcclient:add(conn,nil,on_disconnected)
			for j=1,5000 do
				Sche.Spawn(function (conn)
					while true do			
						local rpccaller = RPC.MakeRPC(conn,"Plus")
						local err,ret = rpccaller:Call(1,2)
						if err then
							print("rpc error:" .. err)
							conn:close()
							return
						else
							count = count + 1
						end
					end
					print("co end")
				end,conn)
			end
		end)	
	end
end)

local tick = GetSysTick()
local now = GetSysTick()
while true do 
	now = GetSysTick()
	if now - tick >= 1000 then
		print("count:" .. count*1000/(now-tick) .. " " .. now-tick)		
		tick = now
		count = 0
	end
	Sche.Sleep(10)
end
