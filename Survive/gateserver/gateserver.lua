local TcpServer = require "lua/tcpserver"
local App = require "lua/application"
local RPC = require "lua/rpc"
local Player = require "gateserver/gateplayer"
local NetCmd = require "netcmd/netcmd"
local MsgHandler = require "netcmd/msghandler"
local Redis = require "lua/redis"
local Sche = require "lua/sche"


local togroup
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
				break
			end
			Sche.Sleep(1000)
		end
	end)	
end

while not togroup or not toredis do
	Sche.Yield()
end





