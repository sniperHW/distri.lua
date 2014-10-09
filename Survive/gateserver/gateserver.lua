local TcpServer = require "lua/tcpserver"
local App = require "lua/application"
local RPC = require "lua/rpc"
local Player = require "Survive/gateserver/gateplayer"
local NetCmd = require "Survive/netcmd/netcmd"
local MsgHandler = require "Survive/netcmd/msghandler"
local Redis = require "lua/redis"
local Sche = require "lua/sche"
local Socket = require "lua/socket"


local togroup
local toredis
local toinner = App.New()
local toclient = App.New()

--建立到redis的连接
local function connect_to_redis()
    if toredis then
		print("to redis disconnected")
    end
    toredis = nil
	Sche.Spawn(function ()
		while true do
			local err
			err,toredis = Redis.Connect("127.0.0.1",6379,connect_to_redis)
			if toredis then
				print("connect to redis success")
				break
			end
			print("try to connect to redis after 1 sec")			
			Sche.Sleep(1000)
		end
	end)	
end


local name2game = {}

local function connect_to_game(name,ip,port)
	print("connect_to_game")
	Sche.Spawn(function ()
		while true do
			local sock = Socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
			if not sock:Connect(ip,port) then
				sock:Establish(CSocket.rpkdecoder(65535))				
				toinner:Add(sock,MsgHandler.onMsg,
							function (s,errno)
								name2game[name] = nil
								print(name .. " disconnected")
								connect_to_game(name,ip,port)
							end)
				local rpccaller = RPC.MakeRPC(sock,"Login")
				local err,ret = rpccaller:Call("gate1")
				if err or ret == "Login failed" then
					if err then
						print(err)
					else
						print(ret)
					end
					sock:Close()
					break							
				end
				print("connect to " .. name .. "success")				
				name2game[name] = sock
				break	
			end
			print("try to connect to " .. name .. "after 1 sec")
			Sche.Sleep(1000)
		end
	end)
end

--有game连接上group
MsgHandler.RegHandler(NetCmd.CMD_GA_NOTIFY_GAME,function (sock,rpk)
	local name = rpk:Read_string()
	if not name2game[name] then
		local ip = rpk:Read_string()
		local port = rpk:Read_uint16()
		connect_to_game(name,ip,port)
	end
end)


local function connect_to_group()
	if togroup then
		print("togroup disconnected")
	end
	togroup = nil
	Sche.Spawn(function ()
		while true do
			local sock = Socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
			if not sock:Connect("127.0.0.1",8811) then
				sock:Establish(CSocket.rpkdecoder(65535))
				toinner:Add(sock,MsgHandler.onMsg,connect_to_group)								
				--登录到groupserver
				local rpccaller = RPC.MakeRPC(sock,"GateLogin")
				local err,ret = rpccaller:Call("gate1")
				if err or ret == "Login failed" then
					if err then
						print(err)
					else
						print(ret)
					end
					sock:Close()
					stop_program()	
				end
				togroup = sock
				print("connect to group success")
				for k,v in pairs(ret) do
					connect_to_game(v[1],v[2],v[3])
				end					
				break
			end
			print("try to connect to group after 1 sec")
			Sche.Sleep(1000)
		end
	end)	
end


connect_to_redis()
connect_to_group()
toinner:Run()
toclient:Run()

while not togroup or not toredis do
	Sche.Yield()
end

if TcpServer.Listen("127.0.0.1",8810,function (sock)
		sock:Establish(CSocket.rpkdecoder(4096))
		toclient:Add(sock,MsgHandler.onMsg,Player.OnPlayerDisconnected)		
	end) then
	print("start server on 127.0.0.1:8810 error")
	stop_program()	
else
	print("start server on 127.0.0.1:8810")
end
