local MinHeap = require "lua/minheap"
local Sche = require "lua/sche"

local timer = {
	minheap,
}

function timer:new(o)
  local o = o or {}   
  setmetatable(o, self)
  self.__index = self
  o.minheap = MinHeap.MinHeap()
  return o
end

function timer:Register(callback,ms,...)
	local t = {}
	t.callback = callback
	t.ms = ms
	t.index = 0
	t.timeout = GetSysTick() + ms
	t.arg = table.pack(...)
	t.timer = self
	self.minheap:Insert(t)
	return self,t
end

function timer:Remove(t)
	if t.timer ~= self or t.invaild then
		return false
	else
		t.invaild = true
		return true
	end
end

function timer:Stop()
	self.stop = true
end

function timer:Run()
	local timer = self.minheap
	while not self.stop do
		local now = GetSysTick()
		while timer:Min() ~= 0 and timer:Min() <= now do
			t = timer:PopMin()
			if not t.invaild then
				local status,ret = pcall(t.callback,table.unpack(t.arg))
				if not status then
					print("timer error:" .. ret)
				elseif ret then
					self:Register(t.callback,t.ms,table.unpack(t.arg))
				end
			end
		end
		Sche.Sleep(1)
	end	
end

return {
	Timer = function () return timer:new() end
}

