local Socket = require "lua.socket"
local App = require "lua.application"
local RPC = require "lua.rpc"
local Sche = require "lua.sche"
local Lock = require "lua.lock"
local NameService = require "examples.rpc.toname"
local RpcHandle = require "examples.rpc.rpchandle"

local name_ip = "127.0.0.1"
local name_port = 8080

local rpcclient = App.New()
NameService.Init(name_ip,name_port)

if NameService.Connect() then
	for i=1,10 do
		Sche.Spawn(function () 
			local rpcHandler = RpcHandle.FindService("Plus")
			if rpcHandler then
				for j=1,100 do
					Sche.Spawn(function ()
						while true do			
							local err,ret = rpcHandler:CallSync(1,2)
							if err == "socket error" then
								rpcHandler = RpcHandle.FindService("Plus")
								if not rpcHandler then
									return
								end
							end
						end
					end)
				end
			end
		end)	
	end
else
	print("connect to name error")
end