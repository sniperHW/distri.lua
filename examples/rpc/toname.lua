local Socket = require "lua.socket"
local App = require "lua.application"
local RPC = require "lua.rpc"
local Sche = require "lua.sche"


local app = App.New()
local sock_toname
local rpc_handle_Find
local rpc_handle_Reg

local nameip,nameport

local private = {}

local public = {}

local service = {}


function private.reconnect()
	Sche.Spawn(function ()
		Sche.Sleep(5000)
		print("reconnect to nameserver")
		if not public.Connect() then
			private.reconnect()
		end
	end)
end


function private.on_disconnected()
	sock_toname = nil
	rpc_handle_Find = nil
	rpc_handle_Reg = nil
	private.reconnect()
	print("on_disconnected")
end

function private.register(name,ip,port)
	if not sock_toname then
		if not public.Connect() then
			return false
		end
	end
	if not rpc_handle_Reg then
		rpc_handle_Reg = RPC.MakeRPC(sock_toname,"Register")
	end

	local err,ret = rpc_handle_Reg:CallSync(name,ip,port)
	if err or not ret then
		print("register to name error")	
		return false
	end
	return true
end

function public.Init(ip,port)
	nameip,nameport = ip,port
end

function public.Connect()
	if sock_toname then
		return false
	end
	sock_toname = Socket.Stream.New(CSocket.AF_INET)
	if sock_toname:Connect(nameip,nameport) then
		sock_toname:Close()
		sock_toname = nil
		print("connect to name error")
		return false
	end
	app:Add(sock_toname:Establish(Socket.Stream.RDecoder(1024),1024),nil,private.on_disconnected,nil,5000) --ping nameserver every 5 sec
	for k,v in pairs(service) do
		if not private.register(table.unpack(v)) then
			sock_toname:Close()
			sock_toname = nil
			return false
		end
	end
	return true
end


function public.Register(name,ip,port)
	if private.register(name,ip,port) then
		table.insert(service,{name,ip,port})
		return true
	end
	return false
end


function public.Find(name)
	if not sock_toname then
		if not public.Connect() then
			return false
		end
	end
	if not rpc_handle_Find then
		rpc_handle_Find = RPC.MakeRPC(sock_toname,"FindService")
	end

	local err,ret = rpc_handle_Find:CallSync(name)
	if err or not ret then
		print("register to name error")	
		return false
	end
	return true,ret
end

return public