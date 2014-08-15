local Sche = require "lua/sche"
local Que  = require "lua/queue"
local socket = {}

function socket:new(domain,type,protocal)
  o = {}
  self.__index = self      
  setmetatable(o,self)
  o.luasocket = luasocket.new1(o,domain,type,protocal)
  o.closing = false
  o.errno = 0
  if not o.luasocket then
		return nil
  else
		return o
  end
end

function socket:new2(sock)
  o = {}
  self.__index = self          
  setmetatable(o, self)
  o.luasocket = luasocket.new2(o,sock)
  o.closing = false
  o.errno = 0
  return o	
end

function socket:close()
	if not self.closing then
		self.closing = true	
		luasocket.close(self.luasocket)
		self.luasocket = nil
	end
end

local function on_new_conn(self,sock)
	print("on_new_conn")
	self.new_conn:push({sock})	
	local co = self.block_onaccept:pop()
	if co then
		print(co.identity)
		Sche.Schedule(co)
	end
end

function socket:listen(ip,port)
	if self.closing then
		return "socket close"
	end
	if self.block_onaccept or self.new_conn then
		return "already listening"
	end
	self.block_onaccept = Que.Queue()
	self.new_conn = Que.Queue()
	self.__on_new_connection = on_new_conn
	return luasocket.listen(self.luasocket,ip,port)
end

local function on_disconnected(self,errno)
	self.errno = errno
	self.closing = true
	if self.block_noaccept then
		while true do
			local co = self.block_onaccept:pop()
			if not co then
				break
			end
			Sche.Schedule(co)
		end
	end
	if self.block_recv then	
		while true do
			local co = self.block_recv:pop()
			if not co then
				break
			end
			print("Sche")
			Sche.Schedule(co)
		end
	end	
	if self.connect_co then
		Sche.Schedule(self.connect_co)
	end
	print("on_disconnected")		
end

local function on_packet(self,packet)
	print("on_packet")
	self.packet:push({packet})
	local co = self.block_recv:pop()
	if co then
	    self.timeout = nil
		Sche.Schedule(co)		
	end
end


function socket:accept(max_packet_size)
	if self.closing then
		return nil,"socket close"
	end

	if not self.block_onaccept or not self.new_conn then
		return nil,"invaild socket"
	else
		
		if not max_packet_size then
			max_packet_size = 65535		
		end
		
		while true do
			local s = self.new_conn:pop()
			if s then
			    s = s[1]
				local sock = socket:new2(s)
				luasocket.establish(sock.luasocket,max_packet_size)
				sock.isestablish = true
				sock.__on_packet = on_packet
				sock.__on_disconnected = on_disconnected
				sock.packet = Que.Queue()
				return sock,nil
			else
				local co = Sche.Running()
				if not co then
					return nil,"accept must be call in a coroutine context"
				end
				self.block_onaccept:push(co)
				Sche.Block()
				print("wake up")
				if  self.closing then
					return nil,"socket close" --socket被关闭
				end				
			end
		end
	end
end

local function cb_connect(self,s,err)
	if not s or err then
		self.err = err
	else
		self.luasocket = socket:new2(s)
	end
	local co = self.connect_co
	self.connect_co = nil
	Sche.Schedule(co)	
end

function socket:connect(ip,port)
	local ret = luasocket.connect(self.luasocket,ip,port)
	if not ret then
		return ret
	else
		local co = Sche.Running()
		if not co then
			return "connect must be call in a coroutine context"
		end
		self.connect_co = co
		self.___cb_connect = cb_connect
		Sche.Block()
		if self.closing then
			return "socket close" 
		elseif self.err then
			return err
		else
			self.isestablish = true
			self.__on_packet = on_packet
			self.__on_disconnected = on_disconnected
			self.packet = Que.Queue()		
			return nil
		end					
	end
end


function socket:recv(timeout)
	if self.closing then
		return nil,"socket close"	
	elseif not self.isestablish then
		return nil,"invaild socket"
	end
	if not self.block_recv then
		self.block_recv = Que.Queue()
	end
	while true do
		local packet = self.packet:pop()
		if packet then
			return packet[1],nil
		end		
		local co = Sche.Running()
		if not co then
			return nil,"recv must be call in a coroutine context"
		end		
		self.block_recv:push(co)
		
		if timeout then
			print("timeout")
			self.timeout = timeout
		end
		Sche.Block(timeout)		
		if self.closing then
			return nil,self.errno
		elseif self.timeout then
		    self.timeout = nil
			return nil,"recv timeout"
		end
	end
end

function socket:send(packet)
	if self.closing then
		return "socket close"
	end
	if not self.luasocket then
		return "invaild socket"
	end
	return luasocket.send(self.luasocket,packet)
end

return {
	new = function (domain,type,protocal) return socket:new(domain,type,protocal) end
}

