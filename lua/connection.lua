local Socket = require "lua/socket"
local Packet = require "lua/packet"

local connection = { }

function connection:new(sock,decoder)
	o = {}
  	self.__index = self          
	setmetatable(o, self)
	o.sock = sock
	o.decoder = decoder
	return o
end

function connection:send(wpk)
	return self.sock:send(wpk.bytebuffer)	
end

function connection:recv()
	while true do		
		local rpacket,err = self.decoder:unpack()
		if err then
			return nil,err
		end

		if rpacket then 
			return rpacket,nil
		end

		local data,err = self.sock:recv()
		if err then		
			return nil,err
		end
		if self.decoder:putdata(data) then
			return nil,err
		end		
	end
end

function connection:close()
	if self.application then
		--唤醒所有等待响应的rpc调用
		self.application.conns[self] = nil
	end
	self.sock:close()
end

return {
	Connection = function (sock,decoder) return connection:new(sock,decoder) end
}
