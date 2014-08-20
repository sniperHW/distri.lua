local cjson = require "cjson"
local Connection = require "lua/connection"
local Sche = require "lua/sche"
local cjson = require "cjson"
local Packet = require "lua/packet"

local CMD_RPC_CALL =  0xABCDDBCA
local CMD_RPC_RESP =  0xDBCAABCD

local function RPC_Process_Call(app,conn,rpk)
	local request = cjson.decode(rpk:read_string())
	local funname = request.f
	local co = request.co
	local func = app._RPCService[funname]
	local response = {co = co}
	if not func then
		response.err = funname .. "not found"
	else
		response.ret = {func(table.unpack(request.arg))}
	end
	local wpk = Packet.WPacket(512)
	wpk:write_uint32(CMD_RPC_RESP)
	wpk:write_string(cjson.encode(response))
	conn:send(wpk)
end

local function RPC_Process_Response(conn,rpk)
	local response = cjson.decode(rpk:read_string())
	local co = Sche.GetCoByIdentity(response.co)
	if co then
		co.response = response
		conn.pending_rpc[co.identity] = nil
		Sche.WakeUp(co)--Schedule(co)
	end
end

local rpcCaller = {}

function rpcCaller:new(conn,funcname)
  local o = {}
  self.__index = self      
  setmetatable(o,self)
  o.conn = conn
  o.funcname = funcname
  return o
end

function rpcCaller:Call(...)
	local request = {}
	local co = Sche.Running()
	request.f = self.funcname
	request.co = co.identity
	request.arg = {...}
	local wpk = Packet.WPacket(512)
	wpk:write_uint32(CMD_RPC_CALL)
	wpk:write_string(cjson.encode(request))
	local ret = self.conn:send(wpk)
	if ret then
		return ret
	else
	    if not self.conn.pending_rpc then
			self.conn.pending_rpc = {}
	    end
		self.conn.pending_rpc[co.identity]  = co
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

local function RPC_MakeCaller(conn,funcname)
	return rpcCaller:new(conn,funcname)
end

return {
	ProcessCall = RPC_Process_Call,
	ProcessResponse = RPC_Process_Response,
	MakeRPC = RPC_MakeCaller,
	CMD_RPC_CALL =  CMD_RPC_CALL,
	CMD_RPC_RESP =  CMD_RPC_RESP,
}
