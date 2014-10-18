local Que = require "lua.queue"
local Shce = require "lua.sche"

local channel = {}

function channel:new()
  local o = {}
  self.__index = self      
  setmetatable(o,self)
  o.chan = Que.New()
  return o
end

function channel:Send(msg)
	self.chan:Push({msg})
	if self.blocking then
		Shce.WakeUp(self.blocking)
	end		
end

function channel:Recv()
	while true do
		local msg = self.chan:Pop()
		if not msg then
			self.blocking = Shce.Running()
			Shce.Block()
			self.blocking = nil
		else
			return msg[1]
		end
	end	
end

return {
	New = function () return channel:new() end
}