--[[
rpc连接只能发送和接收CSocket.rpkdecoder()格式的封包
]]--

local cjson = require "cjson"
local Sche = require "lua.sche"
local Timer = require "lua.timer"

local CMD_RPC_CALL =  0xABCDDBCA
local CMD_RPC_RESP =  0xDBCAABCD
local g_counter = 1
local minheap =  CMinHeap.New()
local timeout_checker

local function init_timeout_checker()
	timeout_checker = Timer.New():Register(function ()
				local contexts = minheap:Pop(C.GetSysTick())
				if contexts then
					for k,v in pairs(contexts) do
						v.on_timeout()
					end
				end
		     	       end,1)
	Sche.Spawn(function() timeout_checker:Run() end)	
end

local function RPC_Process_Call(app,s,rpk)
	local request = rpk:Read_table()
	local funname = request.f
	local func = app._RPCService[funname]
	local response = {identity = request.identity}
	if not func then
		response.err = funname .. " not found"
	else	
		local ret = table.pack(pcall(func,s,table.unpack(request.arg)))
		if ret[1] then
			table.remove(ret,1)			
			response.ret = {table.unpack(ret)}
		else
			response.err = ret[2]
			CLog.SysLog(CLog.LOG_ERROR,string.format("rpc process error:%s",ret[2]))
		end
	end
	local wpk = CPacket.NewWPacket(512)
	wpk:Write_uint32(CMD_RPC_RESP)
	wpk:Write_table(response)
	s:Send(wpk)
end

local function RPC_Process_Response(s,rpk)
	local response = rpk:Read_table()
	if not response then
		CLog.SysLog(CLog.LOG_ERROR,string.format("rpc read table error"))		
		return
	end

	local context = s.pending_call[response.identity]
	if context then
		s.pending_call[response.identity] = nil
		minheap:Remove(context)		
		if context.callback then			
			context.callback(response)
		elseif context.co then
			context.co.response = response
			Sche.WakeUp(context.co)
		end
	end
end

local rpcCaller = {}

function rpcCaller:new(s,funcname)
	local o = {}
	self.__index = self      
	setmetatable(o,self)
	o.s = s
	s.minheap = s.minheap or minheap
	s.pending_call = s.pending_call or {}	
	o.funcname = funcname
	return o
end

function rpcCaller:CallAsync(callback,timeout,...)
	local request = {}
	local co = Sche.Running()
	request.f = self.funcname
	local socket = self.s	
	request.identity = socket:tostring() .. g_counter
	g_counter = g_counter + 1
	request.arg = {...}
	local wpk = CPacket.NewWPacket(512)
	wpk:Write_uint32(CMD_RPC_CALL)
	wpk:Write_table(request)	

	local ret = socket:Send(wpk)
	if ret then
		return "socket error"
	end

	local context = {callback=callback}
	socket.pending_call[request.identity]  = context

	if timeout then
		if not timeout_checker then
			init_timeout_checker()
		end
		context.on_timeout = function()
			callback({err="timeout"})
			socket.pending_call[request.identity] = nil
		end
		minheap:Insert(context,C.GetSysTick() + timeout)
	end
end

function rpcCaller:CallSync(...)
	local request = {}
	local co = Sche.Running()
	local socket = self.s
	request.f = self.funcname
	request.identity = socket:tostring() .. g_counter
	g_counter = g_counter + 1	
	request.arg = {...}
	local wpk = CPacket.NewWPacket(512)
	wpk:Write_uint32(CMD_RPC_CALL)
	wpk:Write_table(request)
	
	local ret = socket:Send(wpk)
	if ret then
		return "socket error"
	end
	local trycount = 1
	local context = {co = co}
	socket.pending_call[request.identity]  = context	
	context.on_timeout = function()
		if trycount <= 2 then
			trycount= trycount + 1
			minheap:Insert(context,C.GetSysTick() + 5000)		
		else
			co.response = {err="timeout"}
			socket.pending_call[request.identity] = nil
			Sche.Schedule(co)
			return
		end
	end
	minheap:Insert(context,C.GetSysTick() + 5000)
	if not timeout_checker then
		init_timeout_checker()
	end	
	Sche.Block()
	local response = co.response
	co.response = nil
	if not response.ret then
		return response.err,nil
	else
		return response.err,table.unpack(response.ret)
	end	
end

local function RPC_MakeCaller(s,funcname)
	return rpcCaller:new(s,funcname)
end

return {
	ProcessCall = RPC_Process_Call,
	ProcessResponse = RPC_Process_Response,
	MakeRPC = RPC_MakeCaller,
	CMD_RPC_CALL =  CMD_RPC_CALL,
	CMD_RPC_RESP =  CMD_RPC_RESP,
}
