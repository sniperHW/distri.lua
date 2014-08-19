local Socket = require "lua/socket"
local Packet = require "lua/packet"
--local Que = require "lua/queue"

local connection = { }

function connection:new(sock,decoder)
	o = {}
  	self.__index = self          
	setmetatable(o, self)
	o.sock = sock
	o.decoder = decoder
	o.closing = false
	return o
end

function connection:send(wpk)
	return self.sock:send(wpk.bytebuffer)	
end

function connection:recv()
	local len,ret,rpacket,err,data
	while true do
		rpacket,err = self.decoder:unpack()	
		if err then
			return nil,err
		elseif rpacket then
			return rpacket,nil
		elseif self.spillover then
			len = string.len(self.spillover)
			ret = self.decoder:putdata(self.spillover,len)
			len = len - ret		
			if len > 0 then
				self.spillover = string.sub(self.spillover,ret+1)
			else
				self.spillover = nil
			end
		else
			data,err = self.sock:recv()
			if err then		
				return nil,err
			end			
			len = string.len(data)
			ret = self.decoder:putdata(data,len)
			len = len - ret
			if len > 0 then
				if self.spillover then
					return nil,"packet too big3"
				end
				self.spillover = string.sub(data,ret+1)
			end	
		end
	end
end

function connection:close()
	if not self.closing then
		if self.application then
			--唤醒所有等待响应的rpc调用
			self.application.conns[self] = nil
		end
		self.sock:close()
		self.closing = true
	end
end

return {
	Connection = function (sock,decoder) return connection:new(sock,decoder) end
}
