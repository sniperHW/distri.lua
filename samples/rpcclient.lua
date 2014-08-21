local Socket = require "lua/socket"
local Packet = require "lua/packet"
local Connection = require "lua/connection"
local App = require "lua/application"
local RPC = require "lua/rpc"
local Sche = require "lua/sche"

local rpcclient = App.New()

rpcclient:Run(function ()
	for i=1,1 do
		Sche.Spawn(function () 
			local client = Socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
			if client:Connect("127.0.0.1",8000) then
				print("connect to 127.0.0.1:8000 error")
				return
			end
			local conn = Connection.New(client,Packet.RPacketDecoder.New(65535))
			rpcclient:Add(conn,nil,on_disconnected)
			for j=1,5000 do
				Sche.Spawn(function (conn)
					while true do			
						local rpccaller = RPC.MakeRPC(conn,"Plus")
						local err,ret = rpccaller:Call(1,2)
						if err then
							print("rpc error:" .. err)
							conn:Close()
							return
						end
					end
					print("co end")
				end,conn)
			end
		end)	
	end
end)
