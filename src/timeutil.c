#include <time.h>
#include <stdint.h>
#include "lua/lua_util.h"

//get timestamp at year:mon:day hour:min
static int lua_GetTS(lua_State *L)
{
	int argsize = lua_gettop(L);
	uint16_t year = 1900;
	uint8_t mon = 1;
	uint8_t day = 1;
	uint8_t hour = 0;
	uint8_t min = 0;
	if(argsize > 0)
		year = (uint16_t)lua_tointeger(L,1);
	if(argsize > 1)
		mon = (uint8_t)lua_tointeger(L,2);
	if(argsize > 2)
		day = (uint8_t)lua_tointeger(L,3);
	if(argsize > 3)
		hour = (uint8_t)lua_tointeger(L,4);					
	if(argsize > 4)
		min = (uint8_t)lua_tointeger(L,5);	
	struct tm local = { 0 };
	local.tm_year = year-1900;
	local.tm_mon = mon-1;
	local.tm_mday = day;
	local.tm_hour = hour;
	local.tm_min = min;
	local.tm_sec = 0;
	lua_pushnumber(L,mktime(&local));
	return 1;
}

//get timestamp at 0:00 of today or the input timestamp
static int lua_GetTSWeeHour(lua_State *L)
{
	time_t timestamp;
	if(lua_gettop(L) > 0)
		timestamp = (time_t)lua_tonumber(L,1);
	else
		timestamp = time(NULL);
	struct tm tmp = { 0 };
	struct tm *local = localtime_r(&timestamp,&tmp);
	local->tm_hour = 0;
	local->tm_min = 0;
	local->tm_sec = 0;
	lua_pushnumber(L,mktime(local));
	return 1;
}

//get timestamp of this monday at 0:00
static int lua_GetWSFirstDay(lua_State *L)
{
	time_t now = time(NULL);
	struct tm tmp = { 0 };
	struct tm *local = localtime_r(&now,&tmp);
	local->tm_hour = 0;
	local->tm_min = 0;
	local->tm_sec = 0;	
	time_t weektime = mktime(local);
	if (local->tm_wday == 0)
	{
		weektime -= 6*24*60*60;
	}
	else
	{
		weektime -= (local->tm_wday-1)*24*60*60;
	}
	lua_pushnumber(L,weektime);
	return 1;	
}

//return diff day of two timestamp
static int lua_DiffDay(lua_State *L)
{
	if(lua_gettop(L) < 2)
		luaL_error(L,"need 2 param");
	time_t a = (time_t)lua_tonumber(L,1);
	time_t b = (time_t)lua_tonumber(L,2);
	struct tm tmp1 = { 0 };
	struct tm tmp2 = { 0 };
	struct tm *_tma = localtime_r(&a,&tmp1);	
	struct tm *_tmb = localtime_r(&b,&tmp2);
	//_tma->tm_hour = 0;
	//_tma->tm_min = 0;
	//_tma->tm_sec = 0;
	//_tmb->tm_hour = 0;
	//_tmb->tm_min = 0;
	//_tmb->tm_sec = 0;
	a = mktime(_tma);
	b = mktime(_tmb);
	lua_pushnumber(L,(int32_t)((b - a)/(3600*24)));
	return 1;
}

//return diff week of two timestamp
static int lua_DiffWeek(lua_State *L)
{
	if(lua_gettop(L) < 2)
		luaL_error(L,"need 2 param");
	time_t a = (time_t)lua_tonumber(L,1);
	time_t b = (time_t)lua_tonumber(L,2);
	struct tm tmp1 = { 0 };
	struct tm tmp2 = { 0 };
	struct tm *_tma = localtime_r(&a,&tmp1);	
	struct tm *_tmb = localtime_r(&b,&tmp2);
	//_tma->tm_hour = 0;
	//_tma->tm_min = 0;
	//_tma->tm_sec = 0;
	//_tmb->tm_hour = 0;
	//_tmb->tm_min = 0;
	//_tmb->tm_sec = 0;
	time_t weektimeA = mktime(_tma);
	time_t weektimeB = mktime(_tmb);
	
	if (_tma->tm_wday == 0)
	{
		weektimeA -= 6*24*60*60;
	}
	else
	{
		weektimeA -= (_tma->tm_wday-1)*24*60*60;
	}

	if (_tmb->tm_wday == 0)
	{
		weektimeB -= 6*24*60*60;
	}
	else
	{
		weektimeB -= (_tmb->tm_wday-1)*24*60*60;
	}
	lua_pushnumber(L,(int32_t)(weektimeB - weektimeA)/(7*24*60*60));
	return 1;
}

//get day of today
static int lua_GetDay(lua_State *L)
{
	time_t now = time(NULL);
	struct tm tmp = { 0 };
	struct tm *local = localtime_r(&now,&tmp);
	lua_pushnumber(L,local->tm_mday);
	return 1;
}

//get month of today
static int lua_GetMon(lua_State *L)
{
	time_t now = time(NULL);
	struct tm tmp = { 0 };
	struct tm *local = localtime_r(&now,&tmp);
	lua_pushnumber(L,(uint32_t)(local->tm_mon+1));
	return 1;
}

//get year of today
static int lua_GetYear(lua_State *L)
{
	time_t now = time(NULL);
	struct tm tmp = { 0 };
	struct tm *local = localtime_r(&now,&tmp);
	lua_pushnumber(L,(uint32_t)(local->tm_year+1900));
	return 1;
}

//get day count of this month
static int lua_GetDayCountOfMon(lua_State *L)
{
	uint16_t year;
	uint8_t mon;
	if(lua_gettop(L) < 2)
		luaL_error(L,"need 2 param");
	year = (uint16_t)lua_tointeger(L,1);
	mon = (uint8_t)lua_tointeger(L,2);
	uint32_t ret = 0;
	switch(mon){
		case 1:{ ret = 31;break;}
		case 2:
			{
				if(year%4==0)
					ret = 29;
				else
					ret = 28;
				break;
			}
		case 3:{ret = 31;break;}
		case 4:{ret = 30;break;}
		case 5:{ret = 31;break;}
		case 6:{ret = 30;break;}
		case 7:
		case 8:
			{
				ret = 31;
				break;
			}
		case 9:{ret = 30;break;}
		case 10:{ret = 31;break;}
		case 11:{ret = 30;break;}
		case 12:{ret = 31;break;}
		default:
			ret = 0;
	}
	lua_pushnumber(L,ret);
	return 1;
}

//get timestamp of the first day of this month at 0:00
static int lua_GetMonFDay(lua_State *L)
{
	time_t now = time(NULL);
	struct tm tmp = { 0 };
	struct tm *local = localtime_r(&now,&tmp);
	local->tm_mday = 0;
	local->tm_hour = 0;
	local->tm_min = 0;
	local->tm_sec = 0;
	lua_pushnumber(L,mktime(local));
	return 1;
}

//get year mon day hour min of the input timestamp
static int lua_GetYearMonDayHourMin(lua_State *L){
	if(lua_gettop(L) < 1)
		luaL_error(L,"need 1 param");
	time_t timestamp = (time_t)lua_tonumber(L,1);
	struct tm tmp = {0};
	struct tm *local = localtime_r(&timestamp,&tmp);
	lua_pushnumber(L,local->tm_year + 1900);
	lua_pushnumber(L,local->tm_mon + 1);
	lua_pushnumber(L,local->tm_mday);
	lua_pushnumber(L,local->tm_hour);
	lua_pushnumber(L,local->tm_min);
	return 5;	
}


//get timestamp of the first day of next month at 0:00
static int lua_GetNextMonFDay(lua_State *L)
{
	time_t now = time(NULL);
	struct tm tmp = { 0 };
	struct tm *local = localtime_r(&now,&tmp);
	local->tm_hour = 0;
	local->tm_min = 0;
	local->tm_sec = 0;
	local->tm_mday = 0;

	if(local->tm_mon == 11)
	{
		local->tm_year += 1;
		local->tm_mon = 0;
	}else
		local->tm_mon += 1;
	lua_pushnumber(L,mktime(local));
	return 1;
}

#define REGISTER_FUNCTION(NAME,FUNC) do{\
	lua_pushstring(L,NAME);\
	lua_pushcfunction(L,FUNC);\
	lua_settable(L, -3);\
}while(0)

void reg_timeutil(lua_State *L){
	lua_newtable(L);		
	REGISTER_FUNCTION("GetTS",lua_GetTS);
	REGISTER_FUNCTION("GetTSWeeHour",lua_GetTSWeeHour);	
	REGISTER_FUNCTION("GetWSFirstDay",lua_GetWSFirstDay);
	REGISTER_FUNCTION("DiffDay",lua_DiffDay);
	REGISTER_FUNCTION("DiffWeek",lua_DiffWeek);	
	REGISTER_FUNCTION("GetDay",lua_GetDay);
	REGISTER_FUNCTION("GetMon",lua_GetMon);
	REGISTER_FUNCTION("GetYear",lua_GetYear);	
	REGISTER_FUNCTION("GetDayCountOfMon",lua_GetDayCountOfMon);	
	REGISTER_FUNCTION("GetMonFDay",lua_GetMonFDay);
	REGISTER_FUNCTION("GetNextMonFDay",lua_GetNextMonFDay);
	REGISTER_FUNCTION("GetYearMonDayHourMin",lua_GetYearMonDayHourMin);
	lua_setglobal(L,"CTimeUtil");		
}