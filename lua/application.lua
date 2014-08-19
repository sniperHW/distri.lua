local Connection = require "lua/connection"
local Sche = require "lua/sche"
local Packet = require "lua/packet"
local Que = require "lua/queue"
local RPC = require "lua/rpc"


--为每个conn启动一个单独的coroutine执行recv
--启动一组coroutine执行on_packet

local application = {}

function application:new(o)
  o = o or {}   
  setmetatable(o, self)
  self.__index = self
  o.conns = {}
  o.running = false
  o.queue = Que.Queue()
  o.blocking = Que.Queue()
  return o
end


local function push(app,conn,rpk)
	app.queue:push({conn,rpk})
	local free = app.blocking:pop()
	if free then
		Sche.Schedule(free)
	end
end

local function recver(app,conn)
	while app.running do
		local rpk,err = conn:recv()
		if err then
			if conn.on_disconnected then
				conn.on_disconnected(conn,err)
			else
				conn:close()
			end
			break
		end
		--将packet投递到队列，供worker消费
		push(conn.application,conn,rpk)	
	end
	app.conns[conn] = nil	
end

function application:add(conn,on_packet,on_disconnected)
	if not self.conns[conn] then
		self.conns[conn] = conn
		conn.application = self
		conn.on_packet = on_packet
		conn.on_disconnected = on_disconnected
		if self.running then
			--启动一个新的coroutine去接收conn的网络数据
			Sche.Spawn(recver,self,conn)
		end
	end
end

local function packet_worker(app)
	local blocking = app.blocking
	local queue = app.queue
	while app.running do		
		local task = queue:pop()
		if not task then
			blocking:push(Sche.Running())
			Sche.Block()	
		else
			local conn = task[1]
			local rpk = task[2]
			if rpk then
				local cmd = rpk:peek_uint32()
				if cmd and cmd == RPC.CMD_RPC_CALL or cmd == RPC.CMD_RPC_RESP then
					--如果是rpc消息，执行rpc处理
					if cmd == RPC.CMD_RPC_CALL then
						rpk:read_uint32()
						RPC.RPC_Process_Call(conn,rpk)
					elseif cmd == RPC.CMD_RPC_RESP then
						rpk:read_uint32()
						RPC.RPC_Process_Response(conn,rpk)
					end						
				elseif conn.on_packet then
					conn.on_packet(conn,rpk)
				end
			end
		end
	end
end

function application:run(start_fun)
	start_fun()
	self.running = true
	for k,v in pairs(self.conns) do
		Sche.Spawn(recver,self,v)
	end
	--启动packet处理coroutine
	for i=0,4096 do
		Sche.Spawn(packet_worker,self)
	end
end

function application:stop()
	self.running = false
	--唤醒所有的blocking
	local block = self.blocking:pop()
	while block do
		Sche.Schedule(block)
		block = self.blocking:pop()
	end	
	for k,v in pairs(self.conns) do
		v:close()
	end
end

return {
	Application =  function () return application:new() end
}
