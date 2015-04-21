local Socket = require "lua.socket"
local App = require "lua.application"
local RPC = require "lua.rpc"
local Sche = require "lua.sche"
local Lock = require "lua.lock"
local NameService = require "examples.rpc.toname"

local app = App.New()


local function MakeSynCaller(handle)
	if not handle then
		return
	end
	return function(...)
		return handle:CallSync(...)
	end
end

local function MakeAsynCaller(handle)
	if not handle then
		return
	end
	return function(callback,...)
		return handle:CallAsync(callback,...)
	end	
end

local function FindService()
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
			local success,ret = NameService.Find(name)
			if not success  or not ret then
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
			app:Add(client:Establish(Socket.Stream.RDecoder(65535),65535),nil,on_disconnected)
			handler = RPC.MakeRPC(client,name)
			service_remote_handlers[name] = handler		
		end
		lock:Unlock()
		return handler
	end
end

local public ={
	Get = FindService(),
	SynCaller = MakeSynCaller,
	AsynCaller = MakeAsynCaller,
}

return public



