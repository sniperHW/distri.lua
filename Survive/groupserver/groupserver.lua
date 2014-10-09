local TcpServer = require "lua/tcpserver"
local App = require "lua/application"
local RPC = require "lua/rpc"
local Player = require "Survive/groupserver/groupplayer"
local NetCmd = require "Survive/netcmd/netcmd"
local MsgHandler = require "Survive/netcmd/msghandler"
local Redis = require "lua/redis"
local Sche = require "lua/sche"
local Gate = require "Survive/groupserver/gate"
local Game = require "Survive/groupserver/game"
local toredis
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

connect_to_redis()

while not toredis do
	Sche.Yield()
end

local groupApp = App.New()

local success

local function on_disconnected(sock,errno)
	if sock.type == "gate" then
		Gate.OnGateDisconnected(sock,errno)
	elseif sock.type == "game" then
		Game.OnGameDisconnected(sock,errno)
	end
end

groupApp:Run(function ()
	success = not TcpServer.Listen("127.0.0.1",8811,function (sock)
		sock:Establish(CSocket.rpkdecoder(65535))
		groupApp:Add(sock,MsgHandler.onMsg,on_disconnected)		
	end)
end)

if not success then
	print("start server on 127.0.0.1:8811 error")
	stop_program()		
else
	--注册Gate模块提供的RPC服务
	Gate.RegRpcService(groupApp)
	--注册Game模块提供的RPC服务	
	Game.RegRpcService(groupApp)
	print("start server on 127.0.0.1:8811")
end

