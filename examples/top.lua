local Sche = require "lua.sche"
local Redis = require "lua.redis"
local err,toredis = Redis.Connect("127.0.0.1",6379,function () print("disconnected") end)
if not err then
	AddTopFilter("distrilua")
	while true do
		local machine_status = Top()
		toredis:Command("set machine " .. CBase64.encode(machine_status))
		print(machine_status)
		Sche.Sleep(1000)
	end
else
	Exit()
end
