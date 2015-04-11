local Socket = require "lua.socket"
local App = require "lua.application"
local RPC = require "lua.rpc"
local Sche = require "lua.sche"

local rpcclient = App.New()

for i=1,10 do
	Sche.Spawn(function () 
		local client = Socket.Stream(CSocket.AF_INET)
		if client:Connect("127.0.0.1",8000) then
			print("connect to 127.0.0.1:8000 error")
			return
		end		
		rpcclient:Add(client:Establish(CSocket.rpkdecoder()),nil,on_disconnected)
		local rpcHandler = RPC.MakeRPC(client,"Plus")
		for j=1,100 do
			Sche.Spawn(function (client)
				while true do			
					local err,ret = rpcHandler:Call(1,2)
					if err then
						print("rpc error:" .. err)
						client:Close()
						return
					end
				end
			end,client)
		end
	end)	
end
