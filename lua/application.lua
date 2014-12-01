local Sche = require "lua.sche"
local RPC = require "lua.rpc"
local Time = require "lua.time"
local application = {}

function application:new(o)
  local o = o or {}   
  setmetatable(o, self)
  self.__index = self
  o.sockets = {}
  o.running = false
  o._RPCService = {}
  return o
end

local CMD_PING = 0xABABCBCB

local function recver(app,socket)
	while app.running do
		local rpk,err = socket:Recv()
		if err then
			socket:Close()
			break
		end
		if rpk then
			if socket.check_recvtimeout then 
				socket.lastrecv = Time.SysTick()
			end
			local cmd = rpk:Peek_uint32()
			if cmd and cmd == RPC.CMD_RPC_CALL or cmd == RPC.CMD_RPC_RESP then
				--如果是rpc消息，执行rpc处理
				if cmd == RPC.CMD_RPC_CALL then
					rpk:Read_uint32()
					RPC.ProcessCall(app,socket,rpk)
				elseif cmd == RPC.CMD_RPC_RESP then
					rpk:Read_uint32()
					RPC.ProcessResponse(socket,rpk)
				end
			elseif cmd and cmd == CMD_PING then
				socket:Send(CPacket.NewWPacket(rpk))		
			elseif socket.process_packet then
				local ret,err = pcall(socket.process_packet,socket,rpk)
				if not ret then
					CLog.SysLog(CLog.LOG_ERROR,err)
				end
			end
		end		
	end
	app.sockets[socket] = nil	
end

function application:Add(socket,on_packet,on_disconnected,recvtimeout,pinginterval)
	if not self.sockets[socket] then
		self.sockets[socket] = socket
		socket.application = self
		socket.process_packet = on_packet
		socket.on_disconnected = on_disconnected
		socket.check_recvtimeout = recvtimeout
		local app = self
		--改变conn.sock.__on_packet的行为
		socket.__on_packet = function (socket,packet)
			socket.packet:Push({packet})
			local co = socket.block_recv:Front()--Pop()
			if co then
				co = co[1]
				socket.timeout = nil
				--Sche.Schedule(co)
				Sche.WakeUp(co)		
			elseif app.running then
				print("SpawnAndRun")
				Sche.SpawnAndRun(recver,app,socket)
			end
		end
		
		if recvtimeout then
			socket.lastrecv = Time.SysTick()
			Sche.Spawn(function ()
				while true do
				              Sche.Sleep(1000)
					if not socket.closing then
						if Time.SysTick() > socket.lastrecv + recvtimeout then
							socket:Close()
							break
						end
					else
						break
					end
				end
			end)	
		end
		
		if pinginterval then
			Sche.Spawn(function ()
				while true do
				    Sche.Sleep(pinginterval)
					if not socket.closing then
						local wpk = CPacket.NewWPacket(64)
						wpk:Write_uint32(CMD_PING)
						socket:Send(wpk)
					else
						break
					end
				end
			end)			
		end	
	end
	return self
end

function application:RPCService(name,func)
	self._RPCService[name] = func
end

function application:Run(start_fun)
	if start_fun then
		start_fun()
	end
	self.running = true
end

function application:Stop()
	self.running = false
	for k,v in pairs(self.sockets) do
		v:Close()
	end
end

return {
	New =  function () return application:new() end
}
