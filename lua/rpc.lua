--[[
rpc连接只能发送和接收CSocket.rpkdecoder()格式的封包
]]--

local cjson = require "cjson"
local Sche = require "lua/sche"

local CMD_RPC_CALL =  0xABCDDBCA
local CMD_RPC_RESP =  0xDBCAABCD

local function RPC_Process_Call(app,s,rpk)
	local str = rpk:Read_string()
	local request = cjson.decode(str)
	local funname = request.f
	local co = request.co
	local func = app._RPCService[funname]
	local response = {co = co}
	if not func then
		response.err = funname .. " not found"
	else
		--response.ret = {func(s,table.unpack(request.arg))}		
		local ret = table.pack(pcall(func,s,table.unpack(request.arg)))
		if ret[1] then
			table.remove(ret,1)			
			response.ret = {table.unpack(ret)}
		else
			response.err = ret[2]
		end
	end
	local wpk = CPacket.NewWPacket(512)--Packet.WPacket.New(512)
	wpk:Write_uint32(CMD_RPC_RESP)
	wpk:Write_string(cjson.encode(response))
	s:Send(wpk)
end

local function RPC_Process_Response(s,rpk)
	local str = rpk:Read_string()
	local response = cjson.decode(str)
	local co = Sche.GetCoByIdentity(response.co)
	if co then
		co.response = response
		s.pending_rpc[co.identity] = nil
		Sche.WakeUp(co)--Schedule(co)
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

function rpcCaller:Call(...)
	local request = {}
	local co = Sche.Running()
	request.f = self.funcname
	request.co = co.identity
	request.arg = {...}
	local wpk = CPacket.NewWPacket(512)--Packet.WPacket.New(512)
	wpk:Write_uint32(CMD_RPC_CALL)
	wpk:Write_string(cjson.encode(request))
	local ret = self.s:Send(wpk)
	if ret then
		return "socket error"
	else
	    if not self.s.pending_rpc then
			self.s.pending_rpc = {}
	    end
		self.s.pending_rpc[co.identity]  = co
		Sche.Block()
		local response = co.response
		co.response = nil
		if not response.ret then
			return response.err,nil
		else
			return response.err,table.unpack(response.ret)
		end
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
