local AlarmClock = require "lua.alarmclock"
local Sche = require "lua.sche"


local function f()
	AlarmClock:SetAlarm({CTimeUtil.GetYearMonDayHourMin(os.time()+60)},
		           function ()
			print("on alarm",CTimeUtil.GetYearMonDayHourMin(os.time()))	
			f()	           		
		           end)
end

f()

while true do
	Sche.Sleep(1000)
end
