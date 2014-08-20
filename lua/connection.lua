local Socket = require "lua/socket"
local Packet = require "lua/packet"
local Sche = require "lua/sche"
--local Que = require "lua/queue"

local connection = { }

function connection:new(sock,decoder)
	local o = {}
  	self.__index = self          
	setmetatable(o, self)
	o.sock = sock
	o.decoder = decoder
	o.closing = false
	sock.__old__on_disconnected = sock.__on_disconnected
	sock.__on_disconnected = function(s,e)
		sock.__old__on_disconnected(s,e)
		if o.pending_rpc then
			--唤醒所有等待响应的rpc调用
			for k,v in pairs(o.pending_rpc) do
				print("process pending")
				v.response = {"remote connection lose",nil}
				Sche.Schedule(v)
			end					
		end
		if o.application then
			o.application.conns[o] = nil
			o.application = nil
		end
		if o.on_disconnected then
			o.on_disconnected(o,e)
		end		
	end
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
	self.closing = true
	self.close = function() end--替换成空函数
	self.sock:close()	
end

return {
	Connection = function (sock,decoder) return connection:new(sock,decoder) end
}
