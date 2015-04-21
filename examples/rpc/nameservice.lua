local TcpServer = require "lua.tcpserver"
local App = require "lua.application"
local RPC = require "lua.rpc"
local Timer = require "lua.timer"
local Sche = require "lua.sche"
local Socket = require "lua.socket"


local service_info = {}

local name_service = App.New() 

name_service:RPCService("FindService",function (name)
	local server = service_info[name]
	if server then
		for k,v in pairs(server) do
			return v
		end
	end
end)

name_service:RPCService("Register",function (name,ip,port,socket)
	local server = service_info[name]
	if not server then
		server = {}
		service_info[name] = server
	end
	print(string.format("%s:%d register %s",ip,port,name))
	server[socket] = {ip,port}
	return true
end)

local function on_disconnected(socket)
	print("on_disconnected")
	for k1,v1 in pairs(service_info) do
		v1[socket] = nil
	end
end

local success = not TcpServer.Listen("127.0.0.1",8080,function (client)		
			name_service:Add(client:Establish(Socket.Stream.RDecoder()),nil,on_disconnected,30000)		
	end)
