local Sche = require "lua/sche"
local Que  = require "lua/queue"
local socket = {}

local function sock_init(sock)
	sock.closing = false
	sock.errno = 0
	return sock
end

function socket:new(domain,type,protocal)
  o = {}
  self.__index = self      
  setmetatable(o,self)
  o.luasocket = luasocket.new1(o,domain,type,protocal)
  if not o.luasocket then
		return nil
  end
  return sock_init(o)   
end

function socket:new2(sock)
  o = {}
  self.__index = self          
  setmetatable(o, self)
  o.luasocket = luasocket.new2(o,sock)
  return sock_init(o)
end

function socket:close()
	if not self.closing then
		self.closing = true	
		luasocket.close(self.luasocket)
		self.luasocket = nil
	end
end

local function on_new_conn(self,sock)
	--print("on_new_conn")
	self.new_conn:push({sock})	
	local co = self.block_onaccept:pop()
	if co then
		--print(co.identity)
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
	local co
	while self.block_noaccept do
		co = self.block_onaccept:pop()
		if co then
			Sche.Schedule(co)
		else
			self.block_noaccept = nil
		end
	end

	while self.block_recv do
		co = self.block_recv:pop()
		if co then
			Sche.Schedule(co) 
		else
			self.block_recv = nil
		end
	end
	
	if self.connect_co then
		Sche.Schedule(self.connect_co)
	end	
end

local function on_packet(self,packet)
	self.packet:push({packet})
	local co = self.block_recv:pop()
	if co then
	    self.timeout = nil
		Sche.Schedule(co)		
	end
end

local function establish(sock,max_packet_size)
	sock.isestablish = true
	sock.__on_packet = on_packet
	sock.__on_disconnected = on_disconnected
	sock.block_recv = Que.Queue()	
	luasocket.establish(sock.luasocket,max_packet_size)
	sock.packet = Que.Queue()	
end


function socket:accept(max_packet_size)
	if self.closing then
		return nil,"socket close"
	end

	if not self.block_onaccept or not self.new_conn then
		return nil,"invaild socket"
	else	
		while true do
			local s = self.new_conn:pop()
			if s then
			    s = s[1]
				local sock = socket:new2(s)
				establish(sock,max_packet_size or 65535)
				return sock,nil
			else
				local co = Sche.Running()
				if not co then
					return nil,"accept must be call in a coroutine context"
				end
				self.block_onaccept:push(co)
				Sche.Block()
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

function socket:connect(ip,port,max_packet_size)
	local ret = luasocket.connect(self.luasocket,ip,port)
	if not ret then
		return ret
	else
		self.connect_co = Sche.Running()
		if not self.connect_co then
			return "connect must be call in a coroutine context"
		end
		self.___cb_connect = cb_connect
		Sche.Block()
		if self.closing then
			return "socket close" 
		elseif self.err then
			return err
		else
			establish(self,max_packet_size or 65535)	
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

