local cjson = require "cjson"
local Connection = require "lua/connection"
local Sche = require "lua/sche"
local cjson = require "cjson"

local CMD_RPC_CALL =  0xABCDDBCA
local CMD_RPC_RESP =  0xDBCAABCD

--在conn上注册一个名为name的rpc服务，服务函数为func
local function RPCServer(conn,name,func)
	local rpcserver = conn.rpcserver
	if not rpcserver then
		conn.rpcserver = {}
		rpcserver = conn.rpcserver
	end
	rpcserver[name] = func
end

local function RPC_Process_Call(conn,rpk)
	rpk:read_uint32()
	rpk = cjson.decode(rpk:read_string())
	local funname = rpk.f
	local co = rpk.co
	local func
	if conn.rpcserver then
		func = conn.rpcserver[funname]
	end
	local response = {co = co}
	if not func then
		response.err = funname .. "not found"
	else
		response.ret = table.pack(func(table.unpack(rpk.arg)))
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
		conn.pending_prc[co] = nil
		Sche.Schedule(co)
	end
end

local rpcCaller = {}

function rpcCaller:new(conn,funcname)
  o = {}
  self.__index = self      
  setmetatable(o,self)
  o.conn = conn
  o.funcname = funcname
  return o
end

function rpcCaller:Call(...)
	
end

local function RPC_MakeCaller(conn,funcname)
	return rpcCaller:new(conn,funcname)
end

return {
	RPCServer = RPCServer,
	RPC_Process_Call = RPC_Process_Call,
	RPC_Process_Response = RPC_Process_Response,
	CMD_RPC_CALL =  CMD_RPC_CALL,
	CMD_RPC_RESP =  CMD_RPC_RESP,
}
