--[[
对CluaSocket的一层lua封装,提供协程化的阻塞接口,使得在应用上以阻塞的方式调用
Recv,Send,Connect,Accept等接口,而实际是异步处理的
]]--

local Sche = require "lua/sche"
local Que  = require "lua/queue"
local socket = {}

local function sock_init(sock)
	sock.closing = false
	sock.errno = 0
	return sock
end

function socket:new(domain,type,protocal)
  local o = {}
  self.__index = self      
  setmetatable(o,self)
  o.luasocket = CSocket.new1(o,domain,type,protocal)
  if not o.luasocket then
		return nil
  end
  return sock_init(o)   
end

function socket:new2(sock)
  local o = {}
  self.__index = self          
  setmetatable(o, self)
  o.luasocket = CSocket.new2(o,sock)
  return sock_init(o)
end

--[[
关闭socket对象，同时关闭底层的luasocket对象,这将导致连接断开。
务必保证对产生的每个socket对象调用Close。
]]--
function socket:Close()
	print("socket:close")
	if not self.closing then
		self.closing = true	
		CSocket.close(self.luasocket)
		self.luasocket = nil
	end
end

local function on_new_conn(self,sock)
	self.new_conn:Push({sock})	
	local co = self.block_onaccept:Pop()
	if co then
		Sche.WakeUp(co)--Schedule(co)
	end
end

--[[
在ip:port上建立TCP监听
]]--
function socket:Listen(ip,port)
	if self.closing then
		return "socket close"
	end
	if self.block_onaccept or self.new_conn then
		return "already listening"
	end
	self.block_onaccept = Que.New()
	self.new_conn = Que.New()
	self.__on_new_connection = on_new_conn
	return CSocket.listen(self.luasocket,ip,port)
end

local function on_disconnected(self,errno)
	self.errno = errno
	self.closing = true
	local co
	while self.block_noaccept do
		co = self.block_onaccept:Pop()
		if co then
			Sche.WakeUp(co)--Schedule(co)
		else
			self.block_noaccept = nil
		end
	end

	while true do
		co = self.block_recv:Pop()
		if co then
			Sche.WakeUp(co)--Schedule(co) 
		else
			break
		end
	end
	
	if self.connect_co then
		Sche.WakeUp(self.connect_co)--Schedule(self.connect_co)
	end	
end

local function on_packet(self,packet)
	self.packet:Push({packet})
	local co = self.block_recv:Front()
	if co then
	    self.timeout = nil
		--Sche.Schedule(co)
		Sche.WakeUp(co)		
	end
end

local function establish(sock,max_packet_size,decoder)
	sock.isestablish = true
	sock.__on_packet = on_packet
	sock.__on_disconnected = on_disconnected
	sock.block_recv = Que.New()	
	if not decoder then
		decoder = CSocket.rawdecoder()
	end
	CSocket.establish(sock.luasocket,max_packet_size,decoder)
	sock.packet = Que.New()	
end

--[[
接受一个TCP连接,并将新连接的接收缓冲设为max_packet_size
]]--
function socket:Accept(max_packet_size,decoder)
	if self.closing then
		return nil,"socket close"
	end

	if not self.block_onaccept or not self.new_conn then
		return nil,"invaild socket"
	else	
		while true do
			local s = self.new_conn:Pop()
			if s then
			    s = s[1]
				local sock = socket:new2(s)
				establish(sock,max_packet_size or 65535,decoder)
				return sock,nil
			else
				local co = Sche.Running()
				if not co then
					return nil,"accept must be call in a coroutine context"
				end
				self.block_onaccept:Push(co)
				Sche.Block()
				if  self.closing then
					return nil,"socket close" --socket被关闭
				end				
			end
		end
	end
end

local function cb_connect(self,s,err)
	if not s or err ~= 0 then
		self.err = err
	else
		self.luasocket = CSocket.new2(self,s)
	end
	local co = self.connect_co
	self.connect_co = nil
	Sche.WakeUp(co)--Schedule(co)	
end

--[[
尝试与ip:port建立TCP连接，如果连接成功建立，将连接的接收缓冲设为max_packet_size
]]--
function socket:Connect(ip,port,max_packet_size,decoder)
	local ret = CSocket.connect(self.luasocket,ip,port)
	if ret then
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
			return self.err
		else
			establish(self,max_packet_size or 65535,decoder)	
			return nil
		end				
	end
end

--[[
尝试从套接口中接收数据,如果成功返回数据,如果失败返回nil,错误描述
timeout参数如果为nil,则当socket没有数据可被接收时Recv调用将一直阻塞
直到有数据可返回或出现错误.否则在有数据可返回或出现错误之前Recv最少阻塞
timeout毫秒
]]--
function socket:Recv(timeout)
	if self.closing then
		return nil,"socket close"	
	elseif not self.isestablish then
		return nil,"invaild socket"
	end
	while true do
		local packet = self.packet:Pop()
		if packet then
			return packet[1],nil
		end		
		local co = Sche.Running()
		if not co then
			return nil,"recv must be call in a coroutine context"
		end		
		self.block_recv:Push(co)		
		if timeout then
			self.timeout = timeout
		end
		Sche.Block(timeout)
		if self.timeout then
		    self.timeout = nil
		    self.block_recv:Remove(co)
			return nil,"recv timeout"
		else
			self.block_recv:Pop()
			if self.closing then
				return nil,self.errno
			end			
		end
	end
end

--[[
将packet发送给对端，如果成功返回nil,否则返回错误描述.
(此函数不会阻塞,立即返回)
]]--
function socket:Send(packet)
	if self.closing then
		return "socket close"
	end
	if not self.luasocket then
		return "invaild socket"
	end
	return CSocket.send(self.luasocket,packet)
end

return {
	New = function (domain,type,protocal) return socket:new(domain,type,protocal) end
}

