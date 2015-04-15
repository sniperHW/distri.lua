--[[
rpc连接只能发送和接收CSocket.rpkdecoder()格式的封包
]]--

local cjson = require "cjson"
local Sche = require "lua.sche"
local Timer = require "lua.timer"
local MinHeap = require "lua.minheap"

local CMD_RPC_CALL =  0xABCDDBCA
local CMD_RPC_RESP =  0xDBCAABCD
local g_counter = 1
local minheap = MinHeap.New()
local timeout_checker

local function RPC_Process_Call(app,s,rpk)
	local request = rpk:Read_table()
	local funname = request.f
	local func = app._RPCService[funname]
	local response = {co = request.co,identity = request.identity}
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
	if response.co then
		local co = Sche.GetCoByIdentity(response.co)
		if co then
			co.response = response
			s.pending_sync_call[co.identity] = nil
			Sche.WakeUp(co)
		end
	elseif response.identity then
		local callback = s.pending_async_call[response.identity]
		if callback then
			callback(response)
			s.pending_async_call[response.identity] = nil
		end
	end
end

local rpcCaller = {}

function rpcCaller:new(s,funcname)
  local o = {}
  self.__index = self      
  setmetatable(o,self)
  o.s = s
  o.funcname = funcname
  return o
end

function rpcCaller:CallAsync(callback,timeout,...)
	local request = {}
	local co = Sche.Running()
	request.f = self.funcname
	request.identity = self.s:tostring() .. g_counter
	g_counter = g_counter + 1
	request.arg = {...}
	local wpk = CPacket.NewWPacket(512)
	wpk:Write_uint32(CMD_RPC_CALL)
	wpk:Write_table(request)	

	local ret = self.s:Send(wpk)
	if ret then
		return "socket error"
	end

	self.s.pending_async_call = self.s.pending_async_call or {}
	self.s.pending_async_call[request.identity]  = callback

	if timeout then
		if not timeout_checker then
			timeout_checker = Timer.New():Register(function ()
				      		local now = C.GetSysTick()
						while minheap:Min() ~=0 and minheap:Min() <= now do
							minheap:PopMin().check()
						end	
				     	       end,1)
			Sche.Spawn(function() timeout_checker:Run() end)
		end
		local checker = {timeout = C.GetSysTick() + timeout,index = 0}
		checker.check = function()
			if not self.s.pending_async_call then
				return
			end
			local callback = self.s.pending_async_call[request.identity]
			if callback then
				callback({err="timeout"})
				self.s.pending_async_call[request.identity] = nil
			end			
		end
		minheap:Insert(checker)
	end
end

function rpcCaller:CallSync(...)
	local request = {}
	local co = Sche.Running()
	request.f = self.funcname
	request.co = co.identity
	request.arg = {...}
	local wpk = CPacket.NewWPacket(512)
	wpk:Write_uint32(CMD_RPC_CALL)
	wpk:Write_table(request)
	
	local ret = self.s:Send(wpk)
	if ret then
		return "socket error"
	end

	self.s.pending_sync_call = self.s.pending_sync_call or {}
	self.s.pending_sync_call[co.identity]  = co
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
