local Connection = require "lua/connection"
local Sche = require "lua/sche"
local Packet = require "lua/packet"
local RPC = require "lua/rpc"

local application = {}

function application:new(o)
  local o = o or {}   
  setmetatable(o, self)
  self.__index = self
  o.conns = {}
  o.running = false
  o._RPCService = {}
  return o
end

local function recver(app,conn)
	while app.running do
		local rpk,err = conn:Recv()
		if err then
			conn:Close()
			break
		end
		--将packet投递到队列，供worker消费
		local cmd = rpk:Peek_uint32()
		if not conn.closing and rpk then
			local cmd = rpk:Peek_uint32()
			if cmd and cmd == RPC.CMD_RPC_CALL or cmd == RPC.CMD_RPC_RESP then
				--如果是rpc消息，执行rpc处理
				if cmd == RPC.CMD_RPC_CALL then
					rpk:Read_uint32()
					RPC.ProcessCall(app,conn,rpk)
				elseif cmd == RPC.CMD_RPC_RESP then
					rpk:Read_uint32()
					RPC.ProcessResponse(conn,rpk)
				end						
			elseif conn.on_packet then
				conn.on_packet(conn,rpk)
			end
		end		
	end
	app.conns[conn] = nil	
end

function application:Add(conn,on_packet,on_disconnected)
	if not self.conns[conn] then
		self.conns[conn] = conn
		conn.application = self
		conn.on_packet = on_packet
		conn.on_disconnected = on_disconnected
		local app = self
		--改变conn.sock.__on_packet的行为
		conn.sock.__on_packet = function (sock,packet)
			sock.packet:Push({packet})
			local co = sock.block_recv:Front()
			if co then
				sock.timeout = nil
				--Sche.Schedule(co)
				Sche.WakeUp(co)		
			elseif app.running then
				print("SpawnAndRun")
				Sche.SpawnAndRun(recver,app,conn)
			end
		end
	end
end

function application:RPCService(name,func)
	self._RPCService[name] = func
end

function application:Run(start_fun)
	start_fun()
	self.running = true
end

function application:Stop()
	self.running = false
	for k,v in pairs(self.conns) do
		v:Close()
	end
end

return {
	New =  function () return application:new() end
}
