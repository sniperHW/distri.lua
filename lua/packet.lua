--[[
提供一组封包处理
]]--


--[[
原始的二进制数据
]]--
local rawpacket = {}

function rawpacket:new(data)
	  local o = {}
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

function rawpacket:Read_rawbin()
	return self.bytebuffer:read_raw(0)
end

function rawpacket:Write_rawbin(data)
	if type(data) == "string" then
		self.bytebuffer:write_raw(0,data)
	end
end

--[[
rpacket和wpacket提供了一组配对的二进制封包格式,其中rpacket用于读取
wpacket用于发送.
封包格式为|4字节数据长度|任意字节长度的数据|
当数据为数字类型时,数据的二进制表示被直接添加到封包的末尾.
如果数据是变长则首先写入4字节的长度,接着是数据内容
]]--

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
	  o.len  = o.bytebuffer:read_uint32(0)
	  o.ridx = 4
	  o.data_remain = o.len  
	  return o
end

function rpacket:Read_uint8()
	 if self.data_remain < 1 then
		return nil
	 end
	 local val = self.bytebuffer:read_uint8(self.ridx) 
	 self.data_reamain = self.data_remain - 1
	 self.ridx = self.ridx + 1
	 return val
end

function rpacket:Read_uint16()
	 if self.data_remain < 2 then
		return nil
	 end
	 local val = self.bytebuffer:read_uint16(self.ridx) 
	 self.data_reamain = self.data_remain - 2
	 self.ridx = self.ridx + 2
	 return val
end

function rpacket:Read_uint32()
	 if self.data_remain < 4 then
		return nil
	 end
	 local val = self.bytebuffer:read_uint32(self.ridx) 
	 self.data_reamain = self.data_remain - 4
	 self.ridx = self.ridx + 4
	 return val
end

function rpacket:Read_float()
	 if self.data_remain < 4 then
		return nil
	 end
	 local val = self.bytebuffer:read_float(self.ridx) 
	 self.data_reamain = self.data_remain - 4
	 self.ridx = self.ridx + 4
	 return val
end

function rpacket:Read_double()
	 if self.data_remain < 8 then
		return nil
	 end
	 local val = self.bytebuffer:read_double(self.ridx) 
	 self.data_reamain = self.data_remain - 8
	 self.ridx = self.ridx + 8
	 return val
end

function rpacket:Read_string()
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

function rpacket:Peek_uint8()
	 if self.data_remain < 1 then
		return nil
	 end
	 local val = self.bytebuffer:read_uint8(self.ridx)
	 return val
end

function rpacket:Peek_uint16()
	 if self.data_remain < 2 then
		return nil
	 end
	 local val = self.bytebuffer:read_uint16(self.ridx)
	 return val
end

function rpacket:Peek_uint32()
	 if self.data_remain < 4 then
		return nil
	 end
	 local val = self.bytebuffer:read_uint32(self.ridx)
	 return val
end

function rpacket:Size()
	return self.len
end

--[[
rpacket解包器,套接字接收到数据时通过调用Putdata将数据添加到解包器中.
之后调用Unpack,如果接收到的数据中有完整的包被解出,则Unpack返回一个rpacket对象.
否则返回nil.需要为解包器的构造函数提供一个maxpacket_size,这意味则解包器最大可
解出的包的数据长度加上包头不可超过maxpacket_size,如果发现有包的大小超过maxpacket_size
则认为出现了错误。


Decoder作为一个通用接口,使用者可以提供自定义的解包器去解符合需求的封包,
只要为自定义的解包器提供Putdata(data,len),和Unpack函数即可.

]]--
local Decoder = {}
function Decoder:new(maxpacket_size)
	  o = {}
	  self.__index = self      
	  setmetatable(o,self)
	  o.maxpacket_size = maxpacket_size
	  o.buffer = CBuffer.bytebuffer(maxpacket_size)
	  o.ridx = 0
	  o.widx = 0
	  return o
end

function Decoder:Putdata(data,len)
	local space = self.maxpacket_size - self.widx
	if len > space and self.ridx > 0 then		
		local datasize = self.widx - self.ridx	
		if datasize > 0 then
			--移动数据
			self.buffer:move(self.ridx,datasize)		
		end
		self.widx = datasize
		self.ridx = 0
		space = self.maxpacket_size - datasize		
	end

	if space >= len then
		self.buffer:write_raw(self.widx,data)
		self.widx = self.widx + len
		return len
	else
		self.buffer:write_raw(self.widx,string.sub(data,1,space))
		self.widx = self.maxpacket_size
		return space				
	end
end

function Decoder:Unpack()
	local datasize = self.widx - self.ridx
	if datasize >= 4 then		
		local packet_len = self.buffer:read_uint32(self.ridx) + 4 --加上包头大小
		if packet_len > self.maxpacket_size then
			return nil,"packet too big " .. packet_len
		elseif packet_len <= datasize then
			--有完整的包
			local rpk = rpacket:new(CBuffer.bytebuffer(self.buffer:read_raw(self.ridx,packet_len)))	
			self.ridx = self.ridx + packet_len			
			return rpk,nil			
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

function wpacket:Write_uint8(val)
	self.bytebuffer:write_uint8(self.widx,val)
	self.widx = self.widx + 1
	self.datasize = self.datasize + 1
	self.bytebuffer:updatesize(self.datasize)
	self.bytebuffer:write_uint32(0,self.datasize-4)--更新包长	
end

function wpacket:Write_uint16(val)
	self.bytebuffer:write_uint16(self.widx,val)
	self.widx = self.widx + 2
	self.datasize = self.datasize + 2
	self.bytebuffer:updatesize(self.datasize)	
	self.bytebuffer:write_uint32(0,self.datasize-4)--更新包长	
end

function wpacket:Write_uint32(val)
	self.bytebuffer:write_uint32(self.widx,val)
	self.widx = self.widx + 4
	self.datasize = self.datasize + 4
	self.bytebuffer:updatesize(self.datasize)	
	self.bytebuffer:write_uint32(0,self.datasize-4)--更新包长	
end


function wpacket:Write_float(val)
	self.bytebuffer:write_float(self.widx,val)
	self.widx = self.widx + 4
	self.datasize = self.datasize + 4
	self.bytebuffer:updatesize(self.datasize)	
	self.bytebuffer:write_uint32(0,self.datasize-4)--更新包长	
end

function wpacket:Write_double(val)
	self.bytebuffer:write_double(self.widx,val)
	self.widx = self.widx + 8
	self.datasize = self.datasize + 8
	self.bytebuffer:updatesize(self.datasize)	
	self.bytebuffer:write_uint32(0,self.datasize-4)--更新包长	
end

function wpacket:Write_string(val)
	self.bytebuffer:write_string(self.widx,val)
	self.widx = self.widx + 4 + string.len(val)
	self.datasize = self.widx
	self.bytebuffer:updatesize(self.datasize)	
	self.bytebuffer:write_uint32(0,self.datasize-4)--更新包长
end

function wpacket:Size()
	return self.bytebuffer:read_uint32(0) --不含长度字段
end


return {
	RawPacket = {New = function (...) return rawpacket:new(...) end},
	RPacket = {New = function (...) return rpacket:new(...) end},
	WPacket = {New = function (...) return wpacket:new(...) end},
	RPacketDecoder = {New = function (maxpacket_size) return Decoder:new(maxpacket_size) end},		
}


