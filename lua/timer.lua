local Sche = require "lua.sche"
local timer = {
	minheap,
}

function timer:new(runImmediate,tickinterval)
  local o = {}   
  setmetatable(o, self)
  self.__index = self
  o.minheap = CMinHeap.New()
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
	t.arg = table.pack(...)
	t.timer = self
	self.minheap:Insert(t,C.GetSysTick() + ms)
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
		local now = C.GetSysTick()
		while not self.stop do
			local t = timer:Pop(now)
			if not t then
				break
			end
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

