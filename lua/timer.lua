local MinHeap = require "lua.minheap"
local Sche = require "lua.sche"
local Time = require "lua.time"
local timer = {
	minheap,
}

function timer:new(runImmediate,tickinterval)
  local o = {}   
  setmetatable(o, self)
  self.__index = self
  o.minheap = MinHeap.New()
  o.tickinterval = tickinterval or 1
  if runImmediate then
  	Sche.SpawnAndRun(function () o:Run() end)
  end
  return o
end

function timer:Register(callback,ms,...)
	local t = {}
	t.callback = callback
	t.ms = ms
	t.index = 0
	t.timeout = Time.SysTick() + ms
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
	if self.running then
		return "timer already running"
	end
	local timer = self.minheap
	self.stop = false
	while true do
		local now = Time.SysTick()
		while not self.stop  and timer:Min() ~= 0 and timer:Min() <= now do
			t = timer:PopMin()
			if not t.invaild then
				local status,ret = pcall(t.callback,table.unpack(t.arg))
				if not status then
					local err = ret
					CLog.SysLog(CLog.LOG_ERROR,"timer error:" .. err)
				else
					if ret == nil then
						self:Register(t.callback,t.ms,table.unpack(t.arg))
					end
				end
			end
		end
		if self.stop  then 
			return nil
		end
		Sche.Sleep(self.tickinterval)
	end	
end

return {
	New = function (runImmediate,tickinterval) return timer:new(runImmediate,tickinterval) end
}

