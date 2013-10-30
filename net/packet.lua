LNUMBER = 1
LSTRING = 2 

wpacket = {
	data = nil
}

function wpacket:new(packet)
  local o = {}   
  self.__index = self
  setmetatable(o, self)
  self.data = {}
  return o
end

function wpacket:write_number(num)
	table.insert(self.data,LNUMBER)
	table.insert(self.data,num)
end

function wpacket:write_string(str)
	table.insert(self.data,LSTRING)
	table.insert(self.data,str)
end

function rpk2wpk(rpacket)
	local wpk = wpacket:new()
	for k,v in pairs(rpacket) do
		if type(v) == "number" then
			wpk:write_number(v)
		else if type(v) == "string" then
			wpk:write_string(v)
		end
	end
end