--[[
对CluaSocket的一层lua封装,提供协程化的阻塞接口,使得在应用上以阻塞的方式调用
Recv,Send,Connect,Accept等接口,而实际是异步处理的
]]--

local Sche = require "lua.sche"
local LinkQue  = require "lua.linkque"
local socket = {}

--function for stream socket
local stream = {}

function stream.on_new_conn(self,sock)
	self.new_conn:Push({sock})	
	local co = self.block_onaccept:Pop()
	if co then
		Sche.WakeUp(co)--Schedule(co)
	end
end

function stream.Listen(self,ip,port)
	if not self.luasocket then
		return "socket close"
	end
	if self.block_onaccept or self.new_conn then
		return "already listening"
	end
	self.block_onaccept = LinkQue.New()
	self.new_conn = LinkQue.New()
	self.__on_new_connection = stream.on_new_conn
	self.Connect = nil
	self.Accept = stream.Accept
	return CSocket.listen(self.luasocket,ip,port)
end

function stream.cb_connect (self,s,err)
	if not s or err ~= 0 then
		self.errno = err
	else
		self.luasocket = CSocket.new2(self,s)
		self.Establish = stream.Establish
	end
	local co = self.connect_co
	if co then
		self.connect_co = nil
		Sche.WakeUp(co)--Schedule(co)
	end
end

function stream.Connect(self,ip,port)
	self.Listen = nil
	self.Accept = nil
	local err,ret = CSocket.connect(self.luasocket,ip,port)
	if err then
		return err
	else
		if ret == 0 then
			self.connect_co = Sche.Running()
			self.___cb_connect = stream.cb_connect
			Sche.Block()
		elseif ret == 1 then
			--success immediately
		end
		if not self.luasocket or  self.errno ~= 0 then
			return self.errno
		else
			return nil
		end				
	end
end

 function stream.Accept(self)
	if not self.luasocket then
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
				sock.Establish = stream.Establish
				return sock,nil
			else
				local co = Sche.Running()
				self.block_onaccept:Push(co)
				Sche.Block()
				if  not self.luasocket then
					return nil,"socket close" --socket被关闭
				end				
			end
		end
	end
end

function stream.Recv(self,timeout)
	--[[
	尝试从套接口中接收数据,如果成功返回数据,如果失败返回nil,错误描述
	timeout参数如果为nil,则当socket没有数据可被接收时Recv调用将一直阻塞
	直到有数据可返回或出现错误.否则在有数据可返回或出现错误之前Recv最少阻塞
	timeout毫秒
	]]--		
	while true do--not self.closing do	
		local packet = self.packet:Pop()
		if packet then
			return packet[1],nil
		elseif not self.luasocket then
			return nil,self.errno
		end		
		local co = Sche.Running()
		co = {co}	
		self.block_recv:Push(co)		
		self.timeout = timeout
		Sche.Block(timeout)
		self.block_recv:Remove(co) --remove from block_recv
		if self.timeout then
			self.timeout = nil
			return nil,"recv timeout"
		end
	end	
end

function stream.Send(self,packet)
	--[[
	将packet发送给对端，如果成功返回nil,否则返回错误描述.
	(此函数不会阻塞,立即返回)
	]]--	
	if not self.luasocket then
		return "socket socket"
	end
	return CSocket.stream_send(self.luasocket,packet)
end

function stream.process_c_disconnect_event(self,errno)
	self.errno = errno
	local co
	while self.block_noaccept do
		co = self.block_onaccept:Pop()
		if co then
			Sche.WakeUp(co)--Schedule(co)
		else
			self.block_noaccept = nil
		end
	end

	if self.connect_co then
		Sche.WakeUp(self.connect_co)--Schedule(self.connect_co)
	end

	while true do
		co = self.block_recv:Pop()
		if co then
			co = co[1]
			Sche.WakeUp(co)--Schedule(co) 
		else
			break
		end
	end
	
	if self.pending_rpc then
		--唤醒所有等待响应的rpc调用
		for k,v in pairs(self.pending_rpc) do
			v.response = {err="remote connection lose",ret=nil}
			Sche.Schedule(v)
		end					
	end
	if self.on_disconnected then
		if Sche.Running() then
			self.on_disconnected(self,errno)
		else
			local s = self
			Sche.Spawn(function ()
					s.on_disconnected(s,errno)
					if s.luasocket then 
						s:Close()
					end
				         end)
			return			
		end
	end
	if self.luasocket then
		self:Close()
	end			
end

function stream.process_c_packet_event(self,packet)
	self.packet:Push({packet})
	local co = self.block_recv:Pop()
	if co then
	    	self.timeout = nil
	    	co = co[1]
		Sche.WakeUp(co)		
	end
end

function stream.Establish(self,decoder,recvbuf_size)
	self.__on_packet = stream.process_c_packet_event
	self.__on_disconnected = stream.process_c_disconnect_event
	self.block_recv = LinkQue.New()	
	recvbuf_size = recvbuf_size or 65535	
	CSocket.establish(self.luasocket,recvbuf_size,decoder)
	self.Send = stream.Send
	self.Recv = stream.Recv
	self.Establish = nil
	self.packet = LinkQue.New()
	return self	
end				

--function for datagram socket
local datagram = {}

function datagram.Listen(self,ip,port)
	if not self.luasocket then
		return "socket close"
	end
	return CSocket.listen(self.luasocket,ip,port)
end

function datagram.Send(self,packet,to)
	if not self.luasocket then
		return "socket socket"
	end
	if not to then
		return "need remote addr"
	end
	return CSocket.datagram_send(self.luasocket,packet,to)
end

function datagram.process_c_packet_event(self,packet,from)
	self.packet:Push({packet,from})
	local co = self.block_recv:Pop()
	if co then
	    	self.timeout = nil
	    	co = co[1]
		Sche.WakeUp(co)		
	end
end

function datagram.Recv(self,timeout)
	--[[
	尝试从套接口中接收数据,如果成功返回数据,如果失败返回nil,错误描述
	timeout参数如果为nil,则当socket没有数据可被接收时Recv调用将一直阻塞
	直到有数据可返回或出现错误.否则在有数据可返回或出现错误之前Recv最少阻塞
	timeout毫秒
	]]--		
	while true do--not self.closing do	
		local packet = self.packet:Pop()
		if packet then
			return packet[1],packet[2],nil
		elseif not self.luasocket then
			return nil,nil,self.errno
		end		
		local co = Sche.Running()
		co = {co}	
		self.block_recv:Push(co)		
		self.timeout = timeout
		Sche.Block(timeout)
		self.block_recv:Remove(co) --remove from block_recv
		if self.timeout then
			self.timeout = nil
			return nil,nil,"recv timeout"
		end
	end	
end


function socket:new(domain,type,recvbuf_size,decoder)
	local o = {}
	self.__index = self      
	setmetatable(o,self)
	recvbuf_size = recvbuf_size or 1024
	o.luasocket = CSocket.new1(o,domain,type,recvbuf_size,decoder)
	if not o.luasocket then
	return nil
	end
	o.errno = 0
	if type == CSocket.SOCK_STREAM then
		o.Listen = stream.Listen
		o.Connect = stream.Connect
	else
		o.Listen = datagram.Listen
		o.Send = datagram.Send
		o.Recv = datagram.Recv
		o.packet = LinkQue.New()
		o.block_recv = LinkQue.New()
		o.__on_packet = datagram.process_c_packet_event
	end
	return o   
end

function socket:new2(sock)
	local o = {}
	self.__index = self          
	setmetatable(o, self)
	o.luasocket = CSocket.new2(o,sock)
	o.errno = 0 
	return o
end

--[[
关闭socket对象，同时关闭底层的luasocket对象,这将导致连接断开。
务必保证对产生的每个socket对象调用Close。
]]--
function socket:Close()
	local luasocket = self.luasocket
	if luasocket then
		self.luasocket = nil
		CSocket.close(luasocket)
	end	
end

function socket:tostring()
	return CSocket.tostring(self.luasocket)
end

return {
	Stream = {
			New = function (domain) return socket:new(domain,CSocket.SOCK_STREAM) end,
			RDecoder = CSocket.rpkdecoder,
			RawDecoder =  CSocket.rawdecoder,			
		},
	Datagram = {
			New = function (domain,recvbuf_size,decoder) return socket:new(domain,CSocket.SOCK_DGRAM,recvbuf_size,decoder) end,
			RDecoder = CSocket.datagram_rpkdecoder,
			RawDecoder =  CSocket.datagram_rawdecoder,
		},
	WPacket = CPacket.NewWPacket,
	RPacket =  CPacket.NewRPacket,
	RawPacket = CPacket.NewRawPacket,
}

