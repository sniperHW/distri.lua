local Redis = require "lua.redis"
local Sche = require "lua.sche"
local Timer = require "lua.timer"


local count = 0
local toredis

local function connect_to_redis()
	print("here")
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
			print("try to connect after 1 sec")
			Sche.Sleep(1000)
		end
	end)	
end


connect_to_redis()

while not toredis do
	Sche.Yield()
end

--for i = 1,1000 do
--	toredis:Command("hmset mail mailid:" .. i .. " " .. i )
--end

--local err,result = toredis:Command("hgetall mail" )
--local size = #result/2
--local key = 1
--local val = 2
--for i = 1,size do
--	print(result[key],result[val])
--	key = key + 2
--	val = val + 2
--end

local err,result = toredis:CommandSync("hmget test nickname")-- avatarid chainfo bag skills everydaysign everydaytask achievement")
print(err,result[1])