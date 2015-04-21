local Socket = require "lua.socket"
local App = require "lua.application"
local RPC = require "lua.rpc"
local Sche = require "lua.sche"
local Lock = require "lua.lock"
local NameService = require "examples.rpc.toname"
local RpcHandle = require "examples.rpc.rpchandle"

NameService.Init("127.0.0.1",8080)

local function client_routine()
	local rpc = RpcHandle.FindService("Plus")
	if not rpc then
		print("no Plus service")
		return
	end
	while true do			
		local err,ret = rpc:CallSync(1,2)
		if err == "socket error" then
			rpc = RpcHandle.FindService("Plus")
			if not rpc then
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
