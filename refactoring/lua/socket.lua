local socket = {}

function socket:new1(domain,type,protocal)
  o = {}   
  setmetatable(o, self)
  self.__index = self
  self.__gc = function () print("gc") end
  print(domain)
  print(type)
  print(protocal)
  o.luasocket = luasocket.new1(o,domain,type,protocal)
  if not o.luasocket then
		return nil
  else
		return o
  end
end

function socket:close()
	if self.luasocket then
		luasocket.close(self.luasocket)
		self.luasocket = nil
	end
end

function socket:connect(ip,port,callback)
	self.on_connect = callback
	return luasocket.connect(self.luasocket,ip,port)
end

function socket:listen(ip,port,callback)
	self.on_new_conn = callback
	return luasocket.listen(self.luasocket,ip,port)
end

function socket:set_packet_callback(callback)
	self.on_packet = callback
end

function socket:set_disconn_callback(callback)
	self.on_disconnected = callback
end

function socket:establish(max_packet_size)
	if self.luasocket then
		return luasocket.establish(self.luasocket,max_packet_size)
	else
		return "invaild socket"
	end
end

function socket:send(packet)
	if self.luasocket then
		return luasocket.send(self.luasocket,packet)
	else
		return "invaild socket"
	end
end
--供C直接回调
function socket:__on_packet(packet)
	if self.on_packet then
		self.on_packet(self,packet)
	end
end

function socket:__cb_connect(errno)
	if self.cb_connect then
		self.cb_connect(self,errno)
	end
end

function socket:__on_disconnected(errno)
	if self.on_disconnected then
		self.on_disconnected(self,errno)
	end	
end

function socket:new2(sock)
  o = {}   
  setmetatable(o, self)
  self.__index = self
  self.__gc = function () print("gc") end  
  o.luasocket = luasocket.new2(o,sock)
  return o	
end

function socket:__on_new_conn(sock)
	if self.on_new_conn then
		local s = socket:new2(sock)
		self.on_new_conn(s)
	end
end

return {
	new = function (domain,type,protocal) return socket:new1(domain,type,protocal) end
}
