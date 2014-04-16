local Sche = require "lua/scheduler"
local Que =  require "lua/queue"
--名字到连接的映射
local name_associate_data = {}

local name2socket = {}               --名字到套接口的映射
local rpc_function = {}              --本服务提供的远程方法
local remotefunc_provider = {}       --远程方法提供者
local service_name                   --本服务的名字
local service_info                   --本服务的基本信息
local name_service                   --名字服务地址信息
local on_disconnected                --与一个服务的连接断开后回调
local msgque = Queue()
local lp_wait_on_msgque = Queue()    --阻塞在提取msg上的light process

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
		name2socket[name] = nil
	end
	C.close(s)
end

--处理rpc响应
local function on_rpc_response(response)
	local lp = Sche.GetLightProcessByIdentity(response.coroidentity)
	if lp and lp.block then
	   lp.block.err = response.err
	   lp.block.ret = response.ret
	   Sche.WakeUp(lp)
	   lp.block = nil
	end 
end

--返回rpc响应
local function rpc_response(name,s,coroidentity,ret,err)
	local response = {
						type = "rpc_response",
						coroidentity=coroidentity,
						err = err,
						ret = ret
					  }
	local remote = s
	if not remote then
		remote = get_remote_by_name(name)
	end				  
	if remote then
		C.send(remote,response,nil)
	end	
end

local function on_data(s,data,err)
	if not data then
		local name = get_name(s)
		if name then
			on_disconnected(name)
		end
		on_disconnect(s)
	else
		local msg = table2str.Str2Table(data)
		if msg.type == "rpc_response" then
			on_rpc_response(msg)
		elseif msg.type == "rpc" then
			--处理远程调用
			local err,ret
			if not msg.func then
				local func = rpc_function[msg.func]
				if func then
					err,ret = func(s,msg.param)
				else
					err = "cannot find remote function:" .. msg.func	
				end
			else
				err = "must privide function name"
			end
			
			if service_name == "nameservice" then
				rpc_response(nil,s,msg.coroidentity,ret,err)
			else
				rpc_response(msg.name,nil,msg.coroidentity,ret,err)
			end		
		elseif msg.type == "msg" then
			--投入到队列
			msgque:push(msg)
			if not lp_wait_on_msgque:is_empty() then
				Sche.WakeUp(lp_wait_on_msgque:pop())
			end
		end
	end
end

local function bind(s,name)
	local ret = C.bind(s,{recvfinish=on_data}) 
	if ret and name then
		name2socket[name] = s
		set_name(s,name)
	end
	return ret
end

local function connect_and_register2name()
	local nservice = connect(name_service,30000)
	if nservice then
		bind(nservice,"nameservice")	
		local ret,err = rpc_call(nservice,"Register",name_service)
		if not err then
			C.close(nservice)
			nservice = nil
		end
	end
	return nservice
end

local get_remote_by_name(name)
	local remote = name2socket[name]
	if not remote then
		local associate_data = name_associate_data[name]
		if not associate_data then
			local tmp = {}
			local self = Sche.GetCurrentLightProcess()
			table.insert(tmp,self)
			if name == "nameservice" then
				remote = connect_and_register2name()
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

local function RPCCall(name,funcname,arguments)
	
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

local function Register2Name(nameaddr)
	name_service = nameaddr
	return connect_and_register2name() ~= nil
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
	--[[local ret = remotefunc_map[funcname]
	if not ret then
		--到名字服务查询提供funcname远程方法的服务名字
		local remote_service,err = RpcCall("nameservice","GetRemoteFunc",{funcname})
		if remote_service then
		    remotefunc_map[funcname] = remote_service
		    ret = remote_service		
		end
	end
	return ret]]--
end

local function RegRPCFunction(name,func) 
	rpc_function[name] = func
end

local function on_newclient(s)
	if not bind(nil,s) then
		C.close(s)
	end
end

local function StartLocalService(local_name,local_socktype,local_addr,cb_disconnected)
	service_info = {
		name = local_name,
		addrinfo = {type = local_socktype,addr = local_addr},
		remote_func = rpc_function
	}
	on_disconnected = cb_disconnected
	return C.listen(IPPROTO_TCP,local_socktype,local_addr,{onaccept=on_newclient})
end

--从消息队列提取消息
local function GetMsg()
	local lp = Sche.GetCurrentLightProcess()
	if not lp then
		return nil,"GetMsg can only be call in a light porcess context"
	end
	if msgque:is_empty() then
		lp_wait_on_msgque:push(lp)
		Sche.Block()
	end
	return msgque:pop()
end	

return {
	StartMainListen = StartMainListen,            --启动主监听
	Register2Name = Register2Name,
	SendMsg = SendMsg,                            --向一个远程服务发送消息 
	RPCCall = RPCCall,                            --调用一个远程方法
	RegRPCFunction = RegRPCFunction,
	StartLocalService = StartLocalService,
	GetMsg = GetMsg,
	GetRemoteFuncProvider = GetRemoteFuncProvider --获取提供远程方法的所有服务的名字
}