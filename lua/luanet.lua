local Sche = require "lua/scheduler"
--名字到连接的映射
local service_map = {}
local remotefunc_map = {}

local name_associate_data = {}

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
		if not C.connect(IPPROTO_TCP,SOCK_STREAM,remote_addr,nil,block,timeout) then
			return nil
		end
		if block.flag == 0 then	   
			Sche.Block()
		end
		return block.sock	
	end
	return nil
end

local function rpc_call(remote,func,arguments)
	local lp = Sche.GetCurrentLightProcess()
	local msg = {
		name = local_name,
		coroidentity = lp.identity,
		type = "rpc",
		func = func,
		param = arguments,
	}
	C.send(remote,msg,nil)
	local block = {}
	lp.block = block
	Sche.Block()
	return block.err,block.ret
end

local function on_disconnect(s)
	local name = get_name(s)
	if name then
		service_map[name] = nil
	end
	C.close(s)
end

local function on_rpc_response(response)
	local lp = Sche.GetLightProcessByIdentity(response.coroidentity)
	if lp and lp.block then
	   lp.block.err = response.err
	   lp.block.ret = response.ret
	   Sche.WakeUp(lp)
	   lp.block = nil
	end 
end

local function on_data(s,data,err)
	if not data then
		on_disconnect(s)
	else
		local msg = table2str.Str2Table(data)
		if msg.type == "rpc_response" then
			on_rpc_response(msg)
		elseif msg.type == "rpc" then
			--处理远程调用
			
			
		elseif msg.type == "msg" then
			--投入到队列
		end
	end
end

local function bind(s,name)
	service_map[name] = s
	set_name(s,name)
	C.bind(s,{recvfinish=on_data})
end

local function on_name_data(s,data,err)
	if not data then
		service_map[name] = nil
		C.close(s)
	else
		local response = table2str.Str2Table(data)
		if response.type == "rpc_response" then
			on_rpc_response(response)
		end
	end
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
						C.bind(remote,name)
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
	
	if not Sche.GetCurrentLightProcess() then
		return nil,"RpcCall should be call in a coroutine"
	end 

	local remote = get_remote_by_name(name)
	if not remote then
		return nil,"cannot communicate to " .. name
	else
		return rpc_call(remote,funcname,arguments)
	end
end

local function connect2name(info)
	local nservice = connect(NameServiceAddr,30000)
	if nservice then
		service_map["nameservice"] = nservice
		set_name(nservice,"nameservice")
		C.bind(nservice,{recvfinish=on_name_data})		
		local ret,err = rpc_call(nservice,funcname,info)
		if not err then
			C.close(nservice)
			nservice = nil
		end
	end
	return nservice
end


local function Register2Name(nameaddr,name,socktype,addr,remotefunc_list)
	NameServiceAddr = nameaddr
	
	local localinfo = {
		name = name,
		addrinfo = {type = socktype,addr = addr},
		remote_func = remotefunc_list
	}
	
	return connect2name(localinfo) ~= nil
end


local function SendMsg(name,msg)
	local remote = get_remote_by_name(name)
	if not remote then
		return "cannot communicate to " .. name
	else
		return C.send(remote,{type = "msg",msg  = msg},nil) 	
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

local function do_rpc_response(name,coroidentity,err,ret)
	local response = {
						type = "rpc_response",
						coroidentity=coroidentity,
						err = err,
						ret = ret
					  }
	
	local remote = get_remote_by_name(name)				  
	if remote then
		C.send(remote,response,nil)
	end	
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

