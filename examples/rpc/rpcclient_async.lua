local Socket = require "lua.socket"
local App = require "lua.application"
local RPC = require "lua.rpc"
local Sche = require "lua.sche"

local rpcclient = App.New()

for i=1,10 do
	Sche.Spawn(function () 
		local client = Socket.Stream.New(CSocket.AF_INET)
		if client:Connect("127.0.0.1",8000) then
			print("connect to 127.0.0.1:8000 error")
			return
		end		
		rpcclient:Add(client:Establish(Socket.Stream.RDecoder()),nil,on_disconnected)
		local rpcHandler = RPC.MakeRPC(client,"Plus")
		local function callback(response)
			if not response.err then
				rpcHandler:CallAsync(callback,1000,1,2)
			end
		end		
		for j=1,100 do			
			rpcHandler:CallAsync(callback,1000,1,2)	
		end
	end)	
end
