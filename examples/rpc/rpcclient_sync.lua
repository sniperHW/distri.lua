local Socket = require "lua.socket"
local App = require "lua.application"
local RPC = require "lua.rpc"
local Sche = require "lua.sche"
local Lock = require "lua.lock"

local name_ip = "127.0.0.1"
local name_port = 8080
local rpcclient = App.New()

local toname = Socket.Stream.New(CSocket.AF_INET)
if toname:Connect(name_ip,name_port) then
	print("connect to name error")
	Exit()
end

rpcclient:Add(toname:Establish(Socket.Stream.RDecoder(1024),1024))

local function FindService()
	local to_name_handler = RPC.MakeRPC(toname,"FindService")
	local service_remote_handlers = {}
	local lock = Lock.New()

	local function on_disconnected(socket)
		lock:Lock()
		for k,v in pairs(service_remote_handlers) do
			if v.s == socket then
				service_remote_handlers[k] = nil
			end
		end
		lock:Unlock()
	end

	return function (name)
		lock:Lock()
		local handler = service_remote_handlers[name]
		if not handler then
			local err,ret = to_name_handler:CallSync(name)
			if err or not ret then
				if not ret then
					print("no server service for " .. name)
				end
				lock:Unlock()
				return nil
			end
			local ip,port = ret[1],ret[2]
			local client = Socket.Stream.New(CSocket.AF_INET)
			if client:Connect(ip,port) then
				lock:Unlock()
				print(string.format("connect to %:%d error",ip,port))
				return nil
			end
			rpcclient:Add(client:Establish(Socket.Stream.RDecoder()),nil,on_disconnected)
			handler = RPC.MakeRPC(client,name)
			service_remote_handlers[name] = handler		
		end
		lock:Unlock()
		return handler
	end
end

FindService = FindService()

for i=1,10 do
	Sche.Spawn(function () 
		local rpcHandler = FindService("Plus")
		if rpcHandler then
			for j=1,100 do
				Sche.Spawn(function ()
					while true do			
						local err,ret = rpcHandler:CallSync(1,2)
						if err == "socket error" then
							--print(err)
							rpcHandler = FindService("Plus")
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