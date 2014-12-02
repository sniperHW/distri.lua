local Sche = require "lua.sche"
local Redis = require "lua.redis"
local Cjson = require "cjson"

local deployment={
	{groupname="central",service={
				{type="ssdb-server",logicname="ssdb-server",conf="ssdb.conf",ip="192.168.0.87"},
		}
	},
	{groupname="group1",service={
			{type="groupserver",logicname="groupserver",ip="192.168.0.87",port="8010"},
			{type="gameserver",logicname="gameserver",ip="192.168.0.87",port="8011"},
			{type="gateserver",logicname="gateserver",ip="192.168.0.87",port="8012"},
		}
	},	
	{groupname="group2",service={
			{type="groupserver",logicname="groupserver",ip="192.168.0.88",port="8010"},
			{type="gameserver",logicname="gameserver",ip="192.168.0.88",port="8011"},
			{type="gateserver",logicname="gateserver",ip="192.168.0.88",port="8012"},
		}
	},
}

local function split(s,separator)
	local ret = {}
	local initidx = 1
	local spidx
	while true do
		spidx = string.find(s,separator,initidx)
		if not spidx then
			break
		end
		table.insert(ret,string. sub(s,initidx,spidx-1))
		initidx = spidx + 1
	end
	if initidx ~= string.len(s) then
		table.insert(ret,string. sub(s,initidx))
	end
	return ret
end

local err,toredis = Redis.Connect("127.0.0.1",6379,function () print("disconnected") end)
if not err then
	toredis:Command("set deployment " .. Cjson.encode(deployment))
	AddTopFilter("distrilua")
	AddTopFilter("ssdb-server")
	while true do
		local machine_status = Top()
		print(machine_status)
		local tb = split(machine_status,"\n")
		toredis:Command("set machine " .. CBase64.encode(Cjson.encode(tb)))		
		Sche.Sleep(1000)
	end
else
	Exit()
end