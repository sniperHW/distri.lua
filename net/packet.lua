WPACKET = 1
RPACKET = 2

rpacket = {
	packet = nil,
	type = RPACKET,
}

function rpacket:finalize()
	ReleaseRpacket(self.packet)	
end

function rpacket:new(packet)
  local o = {}   
  self.__gc = rpacket.finalize
  self.__index = self
  setmetatable(o, self)
  o.packet = packet
  return o
end

function rpacket:read_number()
	PacketReadNumber(self.packet)
end

function rpacket:read_string()
	PacketReadString(self.packet)
end

wpacket = {
	packet = nil,
	type = WPACKET,
}

function wpacket:finalize()
	ReleaseWpacket(self.packet)	
end

function wpacket:new(packet)
  local o = {}   
  self.__gc = wpacket.finalize
  self.__index = self
  setmetatable(o, self)
  if packet and packet.type == RPACKET then
	o.packet = CreateWpacket(rpacket.packet,0)
  else
	o.packet = CreateWpacket(nil,64)
  end
  return o
end

function wpacket:write_number(num)
	PacketWriteNumber(self.packet,num)
end

function wpacket:write_string(str)
	PacketWriteString(self.packet,str)
end