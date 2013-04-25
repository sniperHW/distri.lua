net = {
	engine,
	process_packet,    --处理网络包
	on_accept,         --处理新到连接
	on_connect,
	on_disconnect,     --处理连接关闭
}

function net:listen(ip,port)
	Listen(self.engine,ip,port)
	return self
end

function net:connect(ip,port,timeout)
	AsynConnect(self.engine,ip,port,timeout)
end

function net:finalize()
	DestroyNet(self.engine)
end

function net:new(process_packet,on_accept,on_disconnect,on_connect)
  local o = {}   
  self.__gc = net.finalize
  self.__index = self
  setmetatable(o, self)
  o.engine = CreateNet()
  o.process_packet = process_packet
  o.on_accept = on_accept
  o.on_disconnect = on_disconnect
  o.on_connect = on_connect
  return o
end

function net:run(timeout)
	if self.engine == nil then
		return -1
	end
	local msgs = PeekMsg(self.engine,timeout)
	if msgs ~= nil then
		for k,v in pairs(msgs) do
			local type,con,rpk = v[1],v[2],v[3]
			if type == NEW_CONNECTION and self.on_accept then
				self.on_accept(con)
			elseif type == PROCESS_PACKET then
				if self.process_packet then 
					self.process_packet(con,rpk)
				end		
				ReleaseRpacket(rpk)
			elseif type == DISCONNECT then
				if 1 == ReleaseConnection(con) and self.on_disconnect then
					self.on_disconnect(con)
				end
			elseif type == CONNECT_SUCESSFUL and self.on_connect then
				self.on_connect(con)
			elseif type == CONNECTION_TIMEOUT then
				print("CONNECTION_TIMEOUT")
				ActiveCloseConnection(con)
			else
				return -1
			end				
		end 
	end
	return 0
end
