local AlarmClock = require "lua.alarmclock"
local Sche = require "lua.sche"


local alarm1,err = AlarmClock:SetAlarm({year=2014,
				           mon = 12,
				           day = 9,
				           hour = 18,
				           min = 41},
				           function (alarm)
				           	print("on alarm",CTimeUtil.GetYearMonDayHourMin(os.time()))	
				           end)


AlarmClock:SetAlarm({year=2014,
				           mon = 12,
				           day = 9,
				           hour = 18,
				           min = 42},
				           function (alarm)
				           	print("on alarm",CTimeUtil.GetYearMonDayHourMin(os.time()))	
				           end)

AlarmClock:SetAlarm({year=2014,
				           mon = 12,
				           day = 9,
				           hour = 18,
				           min = 43},
				           function (alarm)
				           	print("on alarm",CTimeUtil.GetYearMonDayHourMin(os.time()))	
				           end)

AlarmClock:SetAlarm({year=2014,
				           mon = 12,
				           day = 9,
				           hour = 18,
				           min = 44},
				           function (alarm)
				           	print("on alarm",CTimeUtil.GetYearMonDayHourMin(os.time()))	
				           end)



AlarmClock:RemoveAlarm(alarm1)

while true do
	Sche.Sleep(1000)
end
