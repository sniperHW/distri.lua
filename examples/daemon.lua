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
	{groupname="测试3区",service={
			{type="groupserver",logicname="groupserver",ip="192.168.0.89",port="8010"},
			{type="gameserver",logicname="gameserver",ip="192.168.0.89",port="8011"},
			{type="gateserver",logicname="gateserver",ip="192.168.0.89",port="8012"},
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
	C.AddTopFilter("distrilua")
	C.AddTopFilter("ssdb-server")
	while true do
		local machine_status = C.Top()
		print(machine_status)
		local tb = split(machine_status,"\n")
		local machine = {}
		local i = 1
		while i <= #tb do
			if tb[i] ~= "process_info" then
				table.insert(machine,tb[i])
			else
				i = i + 1	
				break
			end
			i = i + 1
		end
		local process = {}
		while i <= #tb do
			if tb[i] ~= "" then
				local tmp = {}
				local cols = split(tb[i],",")
				for k,v in pairs(cols) do
					local keyvals = split(v,":")
					tmp[keyvals[1]] = keyvals[2];
				end
				table.insert(process,tmp)
			end
			i = i + 1	
		end


		local str = string.format("hmset MachineStatus 192.168.0.87 %s",CBase64.encode(Cjson.encode({machine,process})))
		toredis:Command(str)
		--toredis:Command("set machine " .. CBase64.encode(Cjson.encode(machine)))
		--toredis:Command("set process " .. CBase64.encode(Cjson.encode(process)))			
		Sche.Sleep(1000)
	end
else
	Exit()
end
