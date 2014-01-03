dofile("queue.lua")

socket = {
	type,   -- data or listen
	msgque,
	sock,
	co,      --协程对象
	status,  --当前的状态
	timeout,
	index,   --在timer中的下标
}

function socket:accept()
	if self.type == "data"
		return nil,"accept error"
	else
		if self.msgque:is_empty() then
			--block
			global_sc:Block(self)
		else
		
		end
	end	
end

function socket:recv(timeout)
	if self.type ~= "data" then
		if self.msgque:is_empty() then
			--block
			global_sc:Block(self,timeout)
		else
		
		end
	else
		return nil,"not data socket"
	end
end

function socket:send(data)
	if self.type ~= "data" then
		SendPacket(self.sock,data)
	else
		return nil,"not data socket"
	end
end

function socket:pushmsg(msg)
	self.msgque:push(msg)
	if self.status == "block" then
		
	end
end

function socket:finalize()

end

function socket:new()
  local o = {}   
  self.__gc = socket.finalize
  self.__index = self
  setmetatable(o, self)
  o.msgque = queue:new()
  return o
end