local Sche = require "lua.sche"
local Timer = require "lua.timer"
local MinHeap = require "lua.minheap"
local LinkQue =  require "lua.linkque"

local alarmclock = {}

function alarmclock:new()
	local o = {}   
	setmetatable(o, self)
	self.__index = self
	o.slot = {}
	return o
end

--[[
alarmtime = {
	year,
	mon,
	day,
	hour,
	min
}
]]--
function alarmclock:SetAlarm(alarmtime,onAlarm,...)
	if not alarmtime or not onAlarm then
		return nil,"invaild argument1"
 	end
 	if not alarmtime.year or not alarmtime.mon or not alarmtime.day then
 		return nil,"invaild argument2"
 	end
 	alarmtime.hour = alarmtime.hour or 0
 	alarmtime.min = alarmtime.min or 0
 	local now = os.time()
 	local alarmstamp = CTimeUtil.GetTS(alarmtime.year,alarmtime.mon,alarmtime.day,alarmtime.hour,alarmtime.min)
 	if alarmstamp <= now then
 		return nil,"can't set alarm in the past time"
 	end
 	local alarm = {
 		fireTime = alarmstamp,
 		onAlarm = onAlarm,
 		isVaild = true,
 		arg = table.pack(...)
 	}
 	if not self.minheap then
 		self.minheap = MinHeap.New()
 		startRun = true
 	end
 	local slot = self.slot[alarmstamp]
 	if not slot then
 		slot = {
			index = 0,
			timeout = alarmstamp,
			alarms = LinkQue.New()
		}
		self.slot[alarmstamp] = slot    	
		self.minheap:Insert(slot)
 	end
 	slot.alarms:Push(alarm)
 	if not self.timer then
 		self.timer = Timer.New("runImmediate",1000):Register(function (o) 
 										o:CheckAlarm() 
 									end,1000,self)
 	end
 	return alarm
end

function alarmclock:CheckAlarm()
	local now = os.time()
	while self.minheap:Min() ~= 0 and self.minheap:Min() <= now do
		local t = self.minheap:PopMin()
		local alarms = t.alarms
		while not alarms:IsEmpty() do
			local alarm = alarms:Pop()
			local status,err = pcall(alarm.onAlarm,alarm,table.unpack(alarm.arg))
			if not status then
				CLog.SysLog(CLog.LOG_ERROR,"alarmclock error:" .. err)
			end
			alarms.isVaild = false
		end
		self.slot[t.timeout] = nil
	end
	return true
end

function alarmclock:RemoveAlarm(alarm)
	local slot = self.slot[alarm.fireTime]
	if slot then
		local ret = slot.alarms:Remove(alarm)
		if ret and slot.alarms:IsEmpty() then
			alarm.isVaild = false
			self.slot[slot.timeout] = nil
		end
		return ret
	else
		return false
	end
end

local AlarmClock = alarmclock:new()
return AlarmClock