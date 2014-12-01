local Sche = require "lua.sche"
local Redis = require "lua.redis"
local Cjson = require "cjson"

--[[local str = CBase64.encode("abcd")
print(str)
print(CBase64.decode(str))
]]--

local deployment = {
	{["id"] = "1",["value"] = "192.168.0.87",["data"] = {
		{["id"] = "1.1",["value"] = "top"},
		{["id"] = "1.2",["value"] = "ssdb"}
	}},
	{["id"] = "2",["value"] = "192.168.0.100",["data"] = {
		{["id"] = "2.1",["value"] = "top1"},
		{["id"] = "2.2",["value"] = "ssdb1"}
	}}	
}


local err,toredis = Redis.Connect("127.0.0.1",6379,function () print("disconnected") end)
if not err then
	--toredis:Command("set deployment " .. Cjson.encode(deployment))
	AddTopFilter("distrilua")
	AddTopFilter("ssdb-server")
	while true do
		local machine_status = Top()
		toredis:Command("set machine " .. CBase64.encode(machine_status))
		print(machine_status)
		Sche.Sleep(1000)
	end
else
	Exit()
end