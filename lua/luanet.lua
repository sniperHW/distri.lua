local Sche = require "lua/scheduler"
--名字到连接的映射
local service_map = {}

local skeleton_map = {}

local remotefunc_map = {}

local name_associate_data = {}

NameServiceAddr = nil

local local_main_addr = nil

local function connect(remote_addr,timeout)
	local proto =  remote_addr.proto
	local socktype = remote_addr.socktype
	if proto == IPPROTO_TCP and socktype == SOCK_STREAM then
		local block = {}
		block.lp = Sche.GetCurrentLightProcess()
		block.sock = nil
		block.flag = 0
		if not block.lp then
			return nil
		end
		block.onconnected = function (s)
								block.flag = 1
								if Sche.GetCurrentLightProcess() != block.lp then
									block.sock = s
									Sche.WakeUp(block.lp)
								end
							end	   
		if not _connect(IPPROTO_TCP,SOCK_STREAM,remote_addr,nil,block,timeout) then
			return nil
		end
		if block.flag == 0 then	   
			Sche.Block()
		end
		return block.sock	
	end
	return nil
end

local function rpc_call(remote,name,arguments)
	local skeleton = skeleton_map[name]
	if skeleton then
		return skeleton_GetAddr(remote,arguments)
	else
		return nil,"unknow remote function"
	end
end

local function RemoteSkeleton(name)
	skeleton_map[name] = skeleton_GetAddr
end

--远程调用的本地代理
local function skeleton_GetInfo(remote,arguments)
	
	
end

RemoteSkeleton("GetInfo")

local function skeleton_GetRemoteFunc(remote,arguments)

end

RemoteSkeleton("GetRemoteFunc")

local function on_disconnect(s)
	local name = get_name(s)
	if name then
		service_map[name] = nil
	end
	close(s)
end

local function on_data(s,data,err)
	if not data then
		on_disconnect(s)
	else
		
	end
end

local function bind(s,name)
	service_map[name] = s
	set_name(s,name)
	_bind(s,{recvfinish=on_data})
end


local function on_name_data(s,data,err)
	if not data then
		service_map[name] = nil
		close(s)
	end
end

local function connect2name()
	local nservice = connect(NameServiceAddr,30000)
	if nservice then
		service_map["nameservice"] = nservice
		set_name(nservice,"nameservice")
		_bind(nservice,{recvfinish=on_name_data})
		
		--向nameservice发送本服务的信息
		
	end
	return nservice
end


local function Register2Name()
	return connect2name() ~= nil
end


local get_remote_by_name(name)
	local remote = service_map[name]
	if not remote then
		local associate_data = name_associate_data[name]
		if not associate_data then
			local tmp = {}
			local self = Sche.GetCurrentLightProcess()
			table.insert(tmp,self)
			if name == "nameservice" then
				remote = connect2name()
			else
				local remote_info,err = RpcCall("nameservice","GetInfo",{name})	
				if remote_info then
					remote = connect(remote_info.addr,30000)
					if remote then
						bind(remote,name)
					end
				end	
			end
			
			associate_data = name_associate_data[name]
			for k,v in associate_data do
				if v ~= self then
					v.remote = remote
					Sche.WakeUp(v.lp)
				end
			end
			name_associate_data[name] = nil
		else
			local block = {}
			block.lp = Sche.GetCurrentLightProcess()
			block.remote = nil
			table.insert(associate_data,block)
			Sche.Block()
			remote = block.remote
		end
	end
	return remote
end

local function RpcCall(name,funcname,arguments)
	local remote = get_remote_by_name(name)
	if not remote then
		return nil,"cannot communicate to " .. name
	else
		return rpc_call(remote,funcname,arguments)
	end
end

local function SendMsg(name,msg)
	local remote = get_remote_by_name(name)
	if not remote then
		return "cannot communicate to " .. name
	else
		return tcp.send(remote,msg)
	end
end

local function GetRemoteFuncProvider(funcname)
	local ret = remotefunc_map[funcname]
	if not ret then
		--到名字服务查询提供funcname远程方法的服务名字
		local remote_service,err = RpcCall("nameservice","GetRemoteFunc",{funcname})
		if remote_service then
		    remotefunc_map[funcname] = remote_service
		    ret = remote_service		
		end
	end
	return ret
end

remotefunc_map["GetRemoteFunc"] = {"nameservice"}
remotefunc_map["GetInfo"] = {"nameservice"}

return {
	StartMainListen = StartMainListen,            --启动主监听
	Register2Name = Register2Name,
	SendMsg = SendMsg,                            --向一个远程服务发送消息 
	RpcCall = RpcCall,                            --调用一个远程方法
	RemoteSkeleton = RemoteSkeleton,              --用于注册远程调用的本地代理
	GetRemoteFuncProvider = GetRemoteFuncProvider --获取提供远程方法的所有服务的名字
}

