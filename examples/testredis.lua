local Redis = require "lua/redis"
local Sche = require "lua/sche"
local Timer = require "lua/timer"

local count = 0

local err,conn = Redis.Connect("127.0.0.1",6379,function (conn)
		print("redis conn disconnected")
	end)
if err then
	print(err)
else	
	for j=1,5000 do
		Sche.Spawn(function ()
			while true do
				local err,result = conn:Command("hgetall global")
				if result then
					count = count + 1
				end			
			end
		end)
	end	
	local last = GetSysTick()
	local timer = Timer.New():Register(function ()
		local now = GetSysTick()
		print("count:" .. count*1000/(now-last) .. " " .. now-last)
		count = 0
		last = now
		return true --return true to register once again	
	end,1000):Run()	
end
	
	
