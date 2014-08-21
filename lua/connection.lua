--[[
对socket的一层封装,提供了RPC的处理和面向封包的Recv
]]--


local Socket = require "lua/socket"
local Packet = require "lua/packet"
local Sche = require "lua/sche"
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

function connection:Send(wpk)
	return self.sock:Send(wpk.bytebuffer)	
end

--[[
面向封包的Recv,如果调用成功返回一个由使用者提供的Decoder解出的完整封包,否则返回nil和错误描述.
如果socket接收到的数据中不能构成完整的封包,Recv将被阻塞,直到timeou或出现错误.
]]--

function connection:Recv(timeout)
	local len,ret,rpacket,err,data
	while true do
		rpacket,err = self.decoder:Unpack()	
		if err then
			return nil,err
		elseif rpacket then
			return rpacket,nil
		elseif self.spillover then
			len = string.len(self.spillover)
			ret = self.decoder:Putdata(self.spillover,len)
			len = len - ret		
			if len > 0 then
				self.spillover = string.sub(self.spillover,ret+1)
			else
				self.spillover = nil
			end
		else
			data,err = self.sock:Recv(timeout)
			if err then		
				return nil,err
			end			
			len = string.len(data)
			ret = self.decoder:Putdata(data,len)
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

function connection:Close()
	self.closing = true
	self.close = function() end--替换成空函数
	self.sock:Close()	
end

return {
	New = function (sock,decoder) return connection:new(sock,decoder) end
}
