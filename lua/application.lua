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
		local rpk,err = conn:recv()
		if err then
			conn:close()
			break
		end
		--将packet投递到队列，供worker消费
		local cmd = rpk:peek_uint32()
		if not conn.closing and rpk then
			local cmd = rpk:peek_uint32()
			if cmd and cmd == RPC.CMD_RPC_CALL or cmd == RPC.CMD_RPC_RESP then
				--如果是rpc消息，执行rpc处理
				if cmd == RPC.CMD_RPC_CALL then
					rpk:read_uint32()
					RPC.ProcessCall(app,conn,rpk)
				elseif cmd == RPC.CMD_RPC_RESP then
					rpk:read_uint32()
					RPC.ProcessResponse(conn,rpk)
				end						
			elseif conn.on_packet then
				conn.on_packet(conn,rpk)
			end
		end		
	end
	app.conns[conn] = nil	
end

function application:add(conn,on_packet,on_disconnected)
	if not self.conns[conn] then
		self.conns[conn] = conn
		conn.application = self
		conn.on_packet = on_packet
		conn.on_disconnected = on_disconnected
		local app = self
		--改变conn.sock.__on_packet的行为
		conn.sock.__on_packet = function (sock,packet)
			sock.packet:push({packet})
			local co = sock.block_recv:front()
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

function application:run(start_fun)
	start_fun()
	self.running = true
end

function application:stop()
	self.running = false
	for k,v in pairs(self.conns) do
		v:close()
	end
end

return {
	Application =  function () return application:new() end
}
