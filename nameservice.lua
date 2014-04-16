--[[
名字服务
1) 通过名字查询对端地址
2) 通过远程方法名查询提供远程方法的服务名字
3) 定时检查注册上来的服务是否依然存在
]]--

local luanet = require "lua/luanet"
local net = require "lua/net"
local table2str = require "lua/table2str"

all_service = {} --注册上来的服务
service_remote_func = {} --注册的远程方法



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
luanet.RegRPCFunction("Register") = Register

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
luanet.RegRPCFunction("GetInfo") = GetInfo

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
luanet.RegRPCFunction("GetRemoteFunc") = GetRemoteFunc

local function service_disconnected(name)
	if name then
		local info = all_service[name]
		for k,v in pairs(info.remote_func) do
			service_remote_func[k][name] = nil
		end
		all_service[name] = nil	
	end
end

luanet.StartLocalService("nameservice",SOCK_STREAM,net.netaddr_ipv4("127.0.0.1",8010),service_disconnected)


