local Sche = require "lua.sche"
local NameService = require "examples.rpc.toname"
local RPC = require "examples.rpc.rpchandle"

NameService.Init("127.0.0.1",8080)

for i=1,10 do
	Sche.Spawn(function () 
		local Plus = RPC.AsynCaller(RPC.Get("Plus"))
		if not Plus then
			print("no Plus service")
			return
		end
		local function callback(err,result)
			if not err then
				Plus(callback,1,2)
			else
				if err == "socket error" then
					Plus = RPC.AsynCaller(RPC.Get("Plus"))
					if not Plus then
						return
					end
				end
			end
		end		
		for j=1,100 do			
			Plus(callback,1,2)	
		end
	end)	
end
