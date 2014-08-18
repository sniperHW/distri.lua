local rawpacket = {}

function rawpacket:new(data)
	  o = {}
	  self.__index = self      
	  setmetatable(o,self)
	  if type(data) == "string" then
			o.bytebuffer = CBuffer.bytebuffer(data)
	  elseif type(data) == "userdata" then 
			o.bytebuffer = data
	  else
			return nil
	  end
	  return o
end

function rawpacket:read_rawbin()
	return self.bytebuffer:read_raw(0)
end

function rawpacket:write_rawbin(data)
	if type(data) == "string" then
		self.bytebuffer:write_raw(0,data)
	end
end

local rpacket = {}

function rpacket:new(data)
	  o = {}
	  self.__index = self      
	  setmetatable(o,self)
	  if type(data) == "userdata" then 
			o.bytebuffer = data
	  else
			return nil
	  end
	  o.len  = o.bytebuffer:read_uint32(0) - 4 --除长度字段外有效数据的长度 
	  o.ridx = 4
	  o.data_remain = o.len  
	  return o
end

function rpacket:read_uint8()
	 if self.data_remain < 1 then
		return nil
	 end
	 local val = self.bytebuffer:read_uint8(self.ridx) 
	 self.data_reamain = self.data_remain - 1
	 self.ridx = self.ridx + 1
	 return val
end

function rpacket:read_uint16()
	 if self.data_remain < 2 then
		return nil
	 end
	 local val = self.bytebuffer:read_uint16(self.ridx) 
	 self.data_reamain = self.data_remain - 2
	 self.ridx = self.ridx + 2
	 return val
end

function rpacket:read_uint32()
	 if self.data_remain < 4 then
		return nil
	 end
	 local val = self.bytebuffer:read_uint32(self.ridx) 
	 self.data_reamain = self.data_remain - 4
	 self.ridx = self.ridx + 4
	 return val
end

function rpacket:read_float()
	 if self.data_remain < 4 then
		return nil
	 end
	 local val = self.bytebuffer:read_float(self.ridx) 
	 self.data_reamain = self.data_remain - 4
	 self.ridx = self.ridx + 4
	 return val
end

function rpacket:read_double()
	 if self.data_remain < 8 then
		return nil
	 end
	 local val = self.bytebuffer:read_double(self.ridx) 
	 self.data_reamain = self.data_remain - 8
	 self.ridx = self.ridx + 8
	 return val
end

function rpacket:read_string()
	 if self.data_remain < 4 then
		return nil
	 end
	 local val = self.bytebuffer:read_string(self.ridx)
	 if val then
		local len = string.len(val)
		self.data_reamain = self.data_remain - 4 - len
		self.ridx = self.ridx + 4 + len
	 end
	 return val
end

function rpacket:size()
	return self.len
end

local Decoder = {}
function Decoder:new(maxpacket_size)
	  o = {}
	  self.__index = self      
	  setmetatable(o,self)
	  o.maxpacket_size = maxpacket_size
	  o.buffer = CBuffer.bytebuffer(maxpacket_size)
	  o.datasize = 0
	  return o
end

function Decoder:putdata(data)
	local len = string.len(data)
	if self.datasize + len > self.maxpacket_size then
		return "packet too big"
	end
	self.buffer:write_raw(self.datasize,data)
	self.datasize = self.datasize + len
	return nil
end

function Decoder:unpack()
	if self.datasize >= 4 then
		local packet_len = self.buffer:read_uint32(0) + 4 --加上包头大小
		if packet_len > self.maxpacket_size then
			return nil,"packet too big"
		elseif packet_len <= self.datasize then
			--有完整的包
			self.datasize = self.datasize - packet_len
			if self.datasize > 0 then
				--调整unpack_buffer中的数据
				self.buffer:move(packet_len,self.datasize)
			end
			return rpacket:new(CBuffer.bytebuffer(self.buffer:read_raw(0,packet_len))),nil			
		end		
	else
		return nil,nil
	end
end

local wpacket = {}

function wpacket:new(data)
  o = {}
  self.__index = self      
  setmetatable(o,self)
  if type(data) == "number" then
		o.bytebuffer = CBuffer.bytebuffer(data)
		o.datasize = 4
   elseif type(data) == "userdata" then 
		o.bytebuffer = data
		o.datasize = data.size()
   else
		return nil
   end
   o.widx = o.datasize
   return o
end

function wpacket:write_uint8(val)
	self.bytebuffer:write_uint8(self.widx,val)
	self.widx = self.widx + 1
	self.datasize = self.datasize + 1
	self.bytebuffer:updatesize(self.datasize)
	self.bytebuffer:write_uint32(0,self.datasize-4)--更新包长	
end

function wpacket:write_uint16(val)
	self.bytebuffer:write_uint16(self.widx,val)
	self.widx = self.widx + 2
	self.datasize = self.datasize + 2
	self.bytebuffer:updatesize(self.datasize)	
	self.bytebuffer:write_uint32(0,self.datasize-4)--更新包长	
end

function wpacket:write_uint32(val)
	self.bytebuffer:write_uint32(self.widx,val)
	self.widx = self.widx + 4
	self.datasize = self.datasize + 4
	self.bytebuffer:updatesize(self.datasize)	
	self.bytebuffer:write_uint32(0,self.datasize-4)--更新包长	
end


function wpacket:write_float(val)
	self.bytebuffer:write_float(self.widx,val)
	self.widx = self.widx + 4
	self.datasize = self.datasize + 4
	self.bytebuffer:updatesize(self.datasize)	
	self.bytebuffer:write_uint32(0,self.datasize-4)--更新包长	
end

function wpacket:write_double(val)
	self.bytebuffer:write_double(self.widx,val)
	self.widx = self.widx + 8
	self.datasize = self.datasize + 8
	self.bytebuffer:updatesize(self.datasize)	
	self.bytebuffer:write_uint32(0,self.datasize-4)--更新包长	
end

function wpacket:write_string(val)
	self.bytebuffer:write_string(self.widx,val)
	self.widx = self.widx + 4 + string.len(val)
	self.datasize = self.widx	
	self.bytebuffer:updatesize(self.datasize)	
	self.bytebuffer:write_uint32(0,self.datasize-4)--更新包长		
end

function wpacket:size()
	return self.bytebuffer:read_uint32(0) --不含长度字段
end


return {
	RawPacket = function (...) return rawpacket:new(...) end,
	RPacket = function (...) return rpacket:new(...) end,
	WPacket = function (...) return wpacket:new(...) end,
	RPacketDecoder = function (maxpacket_size) return Decoder:new(maxpacket_size) end,		
}


