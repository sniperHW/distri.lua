local TcpServer = require "lua/tcpserver"
local App = require "lua/application"
local RPC = require "lua/rpc"
local NetCmd = require "Survive/netcmd/netcmd"
local MsgHandler = require "Survive/netcmd/msghandler"
local Redis = require "lua/redis"
local Sche = require "lua/sche"
local Socket = require "lua/socket"
local Gate = require "Survive/gameserver/gate"


local togroup
local toredis
local gameApp = App.New()

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

Gate.RegRpcService(gameApp)
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
				gameApp:Add(sock,MsgHandler.onMsg,connect_to_group)				
				--登录到groupserver
				local rpccaller = RPC.MakeRPC(sock,"GameLogin")
				local err,ret = rpccaller:Call("game1","127.0.0.1",8812)
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
				break
			end
			print("try to connect to group after 1 sec")
			Sche.Sleep(1000)
		end
	end)	
end


connect_to_redis()
connect_to_group()
gameApp:Run()

while not togroup or not toredis do
	Sche.Yield()
end

if TcpServer.Listen("127.0.0.1",8812,function (sock)
		sock:Establish(CSocket.rpkdecoder(65535))
		gameApp:Add(sock,MsgHandler.onMsg,Gate.OnGateDisconnected)		
	end) then
	print("start server on 127.0.0.1:8812 error")
	stop_program()	
else
	print("start server on 127.0.0.1:8812")
end
