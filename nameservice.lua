--[[
名字服务
1) 通过名字查询对端地址
2) 通过远程方法名查询提供远程方法的服务名字
3) 定时检查注册上来的服务是否依然存在
]]--

local luanet = require "lua/luanet"
local netaddr = require "lua/netaddr"
local table2str = require "lua/table2str"

all_service = {} --注册上来的服务
service_remote_func = {} --注册的远程方法



local function Register(arg)
	print("Register")
	local name = arg.name
	print(name)
	local ret = {}
	if all_service[name] then
		return "name aready exist",{}
	end

	print(arg.info.addr.ip)
	local service_info = {
		s = s,
		--lasttick = C.GetTick(),
		info = arg.info,
		remote_func = arg.remote_func
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
	print(name .. " Register success")
	if rfunc then
		print("RCP func")
		for k,v in pairs(rfunc) do
			print(v)
		end
	end
	return nil,{}
end
luanet.RegRPCFunction("Register",Register)

local function GetInfo(arg)
	print("GetInfo")
	local name = arg.service_name
	local ret = nil
	local err = nil
	
	if name then
		print("GetInfo " .. name)
		local info = all_service[name]
		if info then
			ret = info.info
		else
			err = "cannot find " .. name
		end
	else
		err = "must provide name"
	end
	if err then
		print(err)
	end
	return err,ret 			 
end
luanet.RegRPCFunction("GetInfo",GetInfo)

local function GetRemoteFunc(arg)
	local name = arg[1]
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
luanet.RegRPCFunction("GetRemoteFunc",GetRemoteFunc)

local function service_disconnected(name)
	print("service_disconnected " .. name)
	if name then
		local info = all_service[name]
		if info and info.remote_func then
			for k,v in pairs(info.remote_func) do
				service_remote_func[v][name] = nil
			end
			all_service[name] = nil
		end	
	end
end


print("nameservice start listen on 8010")
luanet.StartLocalService("nameservice",SOCK_STREAM,netaddr.netaddr_ipv4("127.0.0.1",8010),service_disconnected)

