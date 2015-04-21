local Sche = require "lua.sche"
local NameService = require "examples.rpc.toname"
local RPC = require "examples.rpc.rpchandle"

NameService.Init("127.0.0.1",8080)

local function client_routine()
	local Plus = RPC.SynCaller(RPC.Get("Plus"))
	if not Plus then
		print("no Plus service")
		return
	end
	while true do			
		local err,ret = Plus(1,2)
		if err == "socket error" then
			Plus = RPC.SynCaller(RPC.Get("Plus"))
			if not Plus then
				return
			end
		end
	end	
end

if NameService.Connect() then
	for i=1,1000 do
		Sche.Spawn(client_routine)
	end
else
	print("connect to name error")
end
