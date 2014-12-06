local Sche = require "lua.sche"
local Redis = require "lua.redis"
local Cjson = require "cjson"
local Socket = require "lua.socket"

local err
local toredis
local serverip = "192.168.0.87"

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

local process
local localservice

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

--server for php request
local function RunDaemonServer()
	local function FindByLogicname(logicname)
		for k,v in pairs(localservice) do
			if v[3] == logicname then
				return  v
			end
		end
		return nil
	end
	
	local function processMsg(sock,msg)
		print("processMsg")
		local opreq = Cjson.decode(msg);
		local retpk = nil
		while true do
			if opreq.ip ~= "" and opreq.ip ~= serverip then
				break
			end
			if opreq.logicname == '' then
				break
			end
			if opreq.op == "Start" then
				local got = nil
				for i = 1,#process do
					if string.find(process[i].cmd,opreq.logicname,1) then
						got = true
						break
					end
				end
				if got then
					break
				end
				local r = FindByLogicname(opreq.logicname)
				if r then
					local luafile = string.format("SurviveServer/%s/%s.lua",r[2],r[2])
					C.ForkExec("./distrilua",luafile,r[1],r[3])
					retpk = CPacket.NewRawPacket("operation success")
				end
				break
			elseif opreq.op == "Stop" or opreq.op == "Kill" then
				print(opreq.op)
				local got = nil
				for i = 1,#process do
					if string.find(process[i].cmd,opreq.logicname,1) then
						got = process[i]
						break
					end
				end
				if not got then
					break
				end
				print(opreq.op,1)
				if opreq.op == "Stop" then
					C.StopProcess(got.pid)
				else
					C.KillProcess(got.pid)	
				end
				print(opreq.op,2)
				retpk = CPacket.NewRawPacket("operation success")
				break
			else
				break
			end
		end
		if not retpk then
			retpk = CPacket.NewRawPacket("invaild operation")
		end
		sock:Send(retpk)
		sock:Close()
	end

	local server = Socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
	if not server:Listen("127.0.0.1",8800) then
			print("DaemonServer listen on 127.0.0.1 8800")
			while true do
				local client = server:Accept()
				print("new client")
				client:Establish()
				Sche.Spawn(function ()
					local unpackbuff = ''
					while true do
						local packet,err = client:Recv()
						if err then
							print("client disconnected err:" .. err)			
							client:Close()
							return
						end
						processMsg(client,packet:Read_rawbin())
					end
				end)
			end		
	else
		print("create DaemonServer on 127.0.0.1 8800 error")
	end
end


err,toredis = Redis.Connect("127.0.0.1",6379,function () print("disconnected") end)
if not err then

	localservice = {}
	for k1,v1 in pairs(deployment) do
		for k2,v2 in pairs(v1.service) do
			if v2.ip == serverip then
				table.insert(localservice,{v1.groupname,v2.type,v2.logicname})
			end
		end
	end

	for k,v in pairs(localservice) do
		print(v[1],v[2],v[3])
	end

	Sche.Spawn(RunDaemonServer)
	toredis:Command("set deployment " .. Cjson.encode(deployment))
	C.AddTopFilter("distrilua")
	C.AddTopFilter("ssdb-server")
	while true do
		local machine_status = C.Top()
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
		process = {}
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
		collectgarbage("collect")			
		Sche.Sleep(1000)
	end
else
	Exit()
end
