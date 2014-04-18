local Sche = require "lua/scheduler"
local Que =  require "lua/queue"
local Tb2Str = require "lua/table2str"

local name2socket = {}               --名字到套接口的映射
local rpc_function = {}              --本服务提供的远程方法
local remotefunc_provider = {}       --远程方法提供者
local service_info                   --本服务的基本信息
local name_service                   --名字服务地址信息
local on_disconnected                --与一个服务的连接断开后回调
local msgque = Que.Queue()
local lp_wait_on_msgque = Que.Queue()    --阻塞在提取msg上的light process

--forword declare
local get_remote_by_name
local RPCCall

local function connect(remote_addr,timeout)
	--if proto == IPPROTO_TCP and socktype == SOCK_STREAM then
		local block = {}
		block.lp = Sche.GetCurrentLightProcess()
		block.sock = nil
		block.flag = 0
		if not block.lp then
			return nil
		end
		block.onconnected = function (s)
								block.flag = 1
								if Sche.GetCurrentLightProcess() ~= block.lp then
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
	--end
	--return nil
end

local pending_rpc = {} --尚未完成的rpc请求，记录下来，如果远程连接断开，立刻唤醒
local function rpc_call(remote,func,arguments)
	local lp = Sche.GetCurrentLightProcess()
	local msg = {
		name = service_info.name,
		coroidentity = lp.identity,
		type = "rpc",
		func = func,
		param = arguments,
	}
	C.send(remote,Tb2Str.Table2Str(msg),nil)
	local pending = pending_rpc[remote]
	if not pending then
		pending = {}
		pending_rpc[remote] = pending
	end
	pending[lp] = lp
	local block = {}
	lp.block = block
	Sche.Block()
	lp.block = nil
	pending[lp] = nil
	return block.ret,block.err
end

local function disconnect(s)
	local name = C.get_name(s)
	if name then
		name2socket[name] = nil
	end
	--处理没有完成的远程调用
	local pending = pending_rpc[s]
	if pending then
		for _ , v in pairs(pending) do
			if v.block then
				v.block.ret = nil
				v.block.err = "remote disconnect"
				Sche.WakeUp(v)
			end
		end
		pending_rpc[s] = nil
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
	end 
end

local function process_rpc(request)
	local remote = request.sock
	local msg = request.msg
	--处理远程调用
	local err,ret
	if msg.func then
		local func = rpc_function[msg.func]
		if func then
			err,ret = func(msg.param)
			if not err and msg.func == "Register" then
				C.set_name(remote,msg.param.name)
			end
		else
			err = "cannot find remote function:" .. msg.func
		end
	else
		err = "must privide function name"
	end
	
	local response = {
						type         = "rpc_response",
						coroidentity = msg.coroidentity,
						err          = err,
						ret          = ret
					  }
	
	if service_info.name ~= "nameservice" then
		remote = get_remote_by_name(msg.name)
	end			  
	if remote then
		C.send(remote,Tb2Str.Table2Str(response),nil)
	end		
end


--返回rpc响应
local function rpc_response(res)
	local response = {
						type = "rpc_response",
						coroidentity=res.identity,
						err = res.err,
						ret = res.ret
					  }
	local remote = res.sock
	if not remote then
		remote = get_remote_by_name(res.name)
	end				  
	if remote then
		C.send(remote,Tb2Str.Table2Str(response),nil)
	end	
end

local function on_data(s,data,err)
	if not data then
		local name = C.get_name(s)
		if name and on_disconnected then
			on_disconnected(name)
		end
		disconnect(s)
	else
		local msg = Tb2Str.Str2Table(data)
		if msg.type == "rpc_response" then
			on_rpc_response(msg)
		elseif msg.type == "rpc" then
			Sche.Spawn(process_rpc,{sock=s,msg=msg})
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
		C.set_name(s,name)
	end
	return ret
end

local function connect_and_register2name()
	local nservice = connect(name_service,30000)
	if nservice then
		bind(nservice,"nameservice")
		print("connect to nameservice sucess")
		
		local info = {}
		info.name = service_info.name
		info.info = service_info.addrinfo
		info.remote_func = {}
		for k,v in pairs(rpc_function) do
			table.insert(info.remote_func,k)
		end	
		local ret,err = rpc_call(nservice,"Register",info)
		if err then
			C.close(nservice)
			nservice = nil
		end
	end
	return nservice
end



local pending_getremote = {}

--[[
当name2socket[name] == nil的时候，如果多个coroutine同时对同一个name调用get_remote_by_name
会产生逻辑错误.所以当name2socket[name] == nil,coroutine请求对name建立连接时将当前的coroutine
插入到pending_getremote[name]中,后面到来的请求发现pending_getremote[name]不为空则不再请求建立
连接，而是将自己也插入到pending_getremote[name]中.当第一个请求成功或失败，都将唤醒pending_getremote[name]
中的所有coroutine(除自己外).
]]--
get_remote_by_name = function(name)
	local remote = name2socket[name]
	local lp = Sche.GetCurrentLightProcess()
	if not remote then
		local pending = pending_getremote[name]
		if not pending then
			local pending = {}
			table.insert(pending,lp)
			pending_getremote[name] = pending
			if name == "nameservice" then
				remote = connect_and_register2name()
			else
				local remote_info,err = RPCCall("nameservice","GetInfo",{service_name=name})
				print("return from GetInfo")	
				if remote_info then
					remote = connect(remote_info.addr,30000)
					if remote then
						print("connect to " .. name .. "success")
						bind(remote,name)
					end
				else
					print("can not find " .. name)
				end
			end
			
			for k,v in pairs(pending) do
				if v ~= lp then
					v.block.remote = remote
					Sche.WakeUp(v)
				end
			end
			pending_getremote[name] = nil
		else
			print("already pending")
			local block = {}
			block.lp = lp
			block.remote = nil
			lp.block = block
			table.insert(pending,lp)
			Sche.Block()
			remote = block.remote
		end
	--else
	--	print("get remote")
	end
	return remote
end

RPCCall = function(name,funcname,arguments)
	
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
		msg.name = service_info.name
		if not C.send(remote,Tb2Str.Table2Str({type = "msg",msg = msg}),nil) then
			return "error on SendMsg"
		else
			return nil
		end 	
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
	print("RegRPCFunction " .. name)
	rpc_function[name] = func
end

local function on_newclient(s)
	if not bind(s,nil) then
		C.close(s)
	end
end

local function StartLocalService(local_name,local_socktype,local_addr,cb_disconnected)
	service_info = {
		name = local_name,
		addrinfo = {type = local_socktype,addr = local_addr}
	}
	on_disconnected = cb_disconnected
	print(on_disconnected)
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
	return msgque:pop(),nil
end	

return {
	Register2Name = Register2Name,
	SendMsg = SendMsg,                            --向一个远程服务发送消息 
	RPCCall = RPCCall,                            --调用一个远程方法
	RegRPCFunction = RegRPCFunction,
	StartLocalService = StartLocalService,
	GetMsg = GetMsg,
	GetRemoteFuncProvider = GetRemoteFuncProvider --获取提供远程方法的所有服务的名字
}
