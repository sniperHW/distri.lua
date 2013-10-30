net = {
	engine,
	_process_packet = nil,    --处理网络包
	_on_accept = nil,         --处理新到连接
	_on_connect = nil,
	_on_disconnect = nil,     --处理连接关闭
	_on_send_finish = nil,
	_send_timeout = nil,      
	_recv_timeout = nil, 
}

function net:listen(on_accept,ip,port)
	self._on_accept = on_accept
	Listen(self.engine,ip,port)
	return self
end

function net:connect(on_connect,ip,port,timeout)
	self._on_connect = on_connect
	Connect(self.engine,ip,port,timeout)
end

function net:process_packet(connection,rpacket)
	if self._process_packet then
		self._process_packet(connection,rpacket)
	end
end

function net:on_accept(connection)
	if self.on_accept then
		self.on_accept(connection)
	end
end

function net:on_connect(connection)
	if self._on_connect then
		self._on_connect(connection)
	end
end

function net:on_disconnect(connection)
	if self._on_disconnect then
		self._on_disconnect(connection)
	end
end

function net:send_timeout(connection)
	if self._send_timeout then
		self._send_timeout(connection)
	end
end

function net:recv_timeout(connection)
	if self._recv_timeout then
		self._recv_timeout(connection)
	end
end

function net:on_send_finish(connection)
	if self._on_send_finish then
		self._on_send_finish(connection)
	end
end

function net:finalize()
	netservice_delete(self.engine)
end

function net:new(process_packet,on_disconnect)
  local o = {}   
  self.__gc = net.finalize
  self.__index = self
  setmetatable(o, self)
  o._process_packet = process_packet
  o._on_disconnect = on_disconnect
  o.engine = netservice_new(o)
  return o
end

function net:run(timeout)
	if self.engine == nil then
		return -1
	end
	return EngineRun(self.engine,timeout)
end
