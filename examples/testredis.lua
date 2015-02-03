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
	
for j=1,1000 do
	Sche.Spawn(function ()
		while true do
			if toredis then
				local err,result = toredis:CommandSync("get robot1")
				if result then
					count = count + 1
				else
					print(err)
				end
			else
				Sche.Sleep(1000)
			end			
		end
	end)
end	
local last = C.GetSysTick()
local timer = Timer.New():Register(function ()
	local now = C.GetSysTick()
	print("count:" .. count*1000/(now-last) .. " " .. now-last)
	count = 0
	last = now
end,1000):Run()		

