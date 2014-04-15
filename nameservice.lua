--[[
名字服务
1) 通过名字查询对端地址
2) 通过远程方法名查询提供远程方法的服务名字
3) 定时检查注册上来的服务是否依然存在
]]--

all_service = {} --注册上来的服务

service_remote_func = {} --注册的远程方法

local_remote_func = {} --本服务提供的远程方法

s2name = {}

local net = require "lua/net"
local table2str = require "lua/table2str"

local function Register(arg)
	local name = arg.name
	local coroidentity = arg.coroidentity
	local ret = {}
	if all_service[name] then
		return "name aready exist",{}
	end

	local service_info = {
		s = s,
		lasttick = GetTick(),
		info = arg.info
	}
	all_service[name] = service_info
	s2name[s] = name
	--注册远程方法
	local rfunc = arg.remote_func
	if rfunc then
		for k,v in pairs(rfunc) do
			local tmp = service_remote_func[v]
			if not tmp then
				tmp = {}
				service_remote_func[v] = tmp
			end
			tmp[name] = v
		end
	end
	return nil,{}
end
local_remote_func["Register"] = Register

local function GetInfo(arg)
	local name = arg.name
	local ret = nil
	local err = nil
	local info = all_service[name]
	if info then
		ret = info
	else
		err = "cannot find " .. name
	end
	return err,ret 			 
end
local_remote_func["GetInfo"] = GetInfo

local function GetRemoteFunc(arg)
	local name = arg.name
	local ret = {}
	local services = service_remote_func[name]
	if services then
		ret.services = {}
		for k,v in pairs(services) do
			table.insert(ret,k)
		end
	end
	return nil,ret
end
local_remote_func["GetRemoteFunc"] = GetRemoteFunc


local function service_disconnect(s)
	local name = s2name[s]
	if name then
		local info = all_service[name]
		for k,v in pairs(info.remote_func) do
			service_remote_func[k][name] = nil
		end
		all_service[name] = nil
		s2name[s] = nil	
	end
	C.close(s)
end

local function on_data(s,data,err)
	if not data then
		print("a client disconnected")
		service_disconnect(s)
	else
		local argument = table2str.Str2Table(data)
		if argument.type == "rpc" then 
			if not argument.func then
				local func = local_remote_func[argument.func]
				if func then
					local err,ret = func(s,argument.param)
					local response = {
										type = "rpc_response",
										coroidentity=argument.coroidentity,
										err = err,
										ret = ret
									  }
					C.send(s,table2str.Table2Str(response),nil)				  
					end					
				end
			end
		elseif argument.type == "msg" and argument.cmd == "ping" then 
			--心跳包
			local info = all_service[argument.name]
			if info then
				info.lasttick = GetTick()
			end
		end
	end
end

local function on_newclient(s)
	print("on_newclient")
	if not C.bind(s,{recvfinish = on_data})then
		print("bind error")
		C.close(s)
	end
end

C.listen(IPPROTO_TCP,SOCK_STREAM,net.netaddr_ipv4("127.0.0.1",8010),{onaccept=on_newclient})

