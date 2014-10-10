local NetCmd = require "Survive/netcmd/netcmd"
local App = require "lua/application"
local socket = require "lua/socket"
local sche = require "lua/sche"
local MsgHandler = require "Survive/netcmd/msghandler"


local Robot = App.New()

MsgHandler.RegHandler(NetCmd.CMD_GC_CREATE,function (sock,rpk)
	print("CMD_GC_CREATE")
	local wpk = CPacket.NewWPacket(64)
	wpk:Write_uint16(NetCmd.CMD_CG_CREATE)	
	wpk:Write_uint8(1)
	wpk:Write_string(sock.actname)
	wpk:Write_uint8(1)
	sock:Send(wpk)
end)


MsgHandler.RegHandler(NetCmd.CMD_GC_BEGINPLY,function (sock,rpk)
	print("CMD_GC_BEGINPLY")
end)

Robot:Run(function ()
	for i=100,100 do
		sche.Spawn(function () 
			local client = socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
			if client:Connect("127.0.0.1",8810) then
				print("connect to 127.0.0.1:8810 error")
				return
			end
			client:Establish(CSocket.rpkdecoder())
			Robot:Add(client,MsgHandler.OnMsg,function () print("robot" .. i .. " disconnected") end)
			local wpk = CPacket.NewWPacket(64)
			wpk:Write_uint16(NetCmd.CMD_CA_LOGIN)
			wpk:Write_uint8(1)
			wpk:Write_string("robot" .. i)
			client.actname = "robot" .. i
			client:Send(wpk)
		end)	
	end
end)



