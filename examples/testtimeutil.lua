print(CTimeUtil.GetTSWeeHour())
print(CTimeUtil.GetTS(2014,12,9))
print(CTimeUtil.GetYear())
print(CTimeUtil.GetMon())
print(CTimeUtil.GetDay())
print(CTimeUtil.DiffDay(CTimeUtil.GetWSFirstDay(),CTimeUtil.GetTSWeeHour()))
print(CTimeUtil.DiffDay(CTimeUtil.GetTS(2014,12,2),CTimeUtil.GetTSWeeHour()))
print(CTimeUtil.DiffWeek(CTimeUtil.GetTS(2014,12,2),CTimeUtil.GetTSWeeHour()))
print(CTimeUtil.GetDayCountOfMon(2014,12))
print(CTimeUtil.GetYearMonDayHourMin(os.time()))



local a = CTimeUtil.GetTS(2014,11,1,0,15)
local b = CTimeUtil.GetTS(2014,12,1)

print(CTimeUtil.DiffDay(a,b))

Exit()