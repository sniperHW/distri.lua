--线程间通信通道
local Que =  require "lua/queue"
local Sche = require "lua/scheduler"
local cjson = require "cjson"

local msgque = Que.Queue()
local callque = Que.Queue()

local lp_wait_on_msgque =  Que.Queue() --阻塞在提取msg上的light process
local lp_wait_on_callque = Que.Queue()

local thread_function = {}             --本线程提供的线程异步方法

local function Fork(start_file)
	local thread = {}
	thread.thread,thread.channel = C.fork(start_file)
	return thread
end

local function Join(thread)
	C.thread_join(thread.thread)
end

local function quepush(que,waitque,ele)
	que:push(ele)
	local lp = waitque:pop()
	if lp then
		Sche.WakeUp(lp)
	end
end

local function quepop(lp,que,waitque)
	local request = que:pop()
	while not request do
		waitque:push(lp)
		Sche.Block()
		request = que:pop()
	end
	return request	
end

--处理call响应
local function on_call_response(response)
	local lp = Sche.GetLightProcessByIdentity(response.i)
	if lp and lp.block then
	   lp.block.err = response.u.err
	   lp.block.ret = response.u.ret
	   Sche.WakeUp(lp)
	end 
end

local function process_call(request)
	local from = request.from
	local msg = request.msg
	local err,ret
	local f = msg.f
	if f then
		local func = thread_function[f]
		if func then
			err,ret = func(msg.u)
		else
			err = "cannot find thd function:" .. f
		end
	else
		err = "must privide function name"
	end
	
	local response = {
						t         = "call_response",
						i         =  msg.i,
						u         = {ret=ret,err=err}
					 }
	
	C.channel_send(from,cjson.encode(response))		
end

local function On_channel_msg(channel,data)
	local msg = cjson.decode(data)
	local type = msg.t
	if type == "call_response" then
		on_call_response(msg)
	elseif type == "call" then
		quepush(callque,lp_wait_on_callque,{from=channel,msg=msg})
	elseif msg.t == "msg" then
		quepush(msgque,lp_wait_on_msgque,{from=channel,msg=msg})		
	end
end

--提取线程队列的消息
local function GetMsg()
	local lp = Sche.GetCurrentLightProcess()
	if not lp then
		return nil,"GetMsg can only be call in a light porcess context"
	end
	return quepop(lp,msgque,lp_wait_on_msgque)
end

--向线程发消息
local function SendMsg(thread,msg)
	return C.channel_send(thread.channel,cjson.encode({t = "msg", u = msg}))
end

--调用其它线程的方法
local function Call(thread,func,arguments)
	local lp = Sche.GetCurrentLightProcess()
	local msg = {
		i = lp.identity,
		t = "call",
		f = func,
		u = arguments,
	}
	local err = C.channel_send(thread.channel,cjson.encode(msg))
	if not err then
		local block = {}
		lp.block = block
		Sche.Block()
		lp.block = nil
		return block.ret,block.err
	else
		return nil,err
	end
end

local function getCallRequest()
	local lp = Sche.GetCurrentLightProcess()
	return quepop(lp,callque,lp_wait_on_callque)	
end	

local function Setup()
	C.setup_channel()
	--启动一组light process用于处理调用
	for i=1,8192 do
		Sche.Spawn(
			function() 
				while true do
					local request = getCallRequest()
					process_call(request)
				end	
			end		
		  )
	end	
end

local function RegThdFunction(name,func)
	print("RegThdFunction " .. name)
	thread_function[name] = func
	print(thread_function[name])
end

return {
	On_channel_msg = On_channel_msg,
	GetMsg = GetMsg,
	SendMsg = SendMsg,
	Call = Call,
	Fork = Fork,
	Setup = Setup,
	RegThdFunction = RegThdFunction,
	Join = Join
}
