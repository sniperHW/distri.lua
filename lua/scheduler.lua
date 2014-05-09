local LightProcess = require "lua/lightprocess"
local Timer = require "lua/timer"
local Que =  require "lua/queue"

local lightprocesses = {}

local scheduler =
{
    runable,
    timer,
    CoroCount,
    current_lp
}

function scheduler:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function scheduler:init()
    self.m_timer = Timer.Timer()
    self.runable = Que.Queue()
    self.current_lp = nil
    self.CoroCount = 0
end

function scheduler:Add2Active(lprocess)
    if lprocess.status == "actived" then
        return
    end
    lprocess.status = "actived"
    self.runable:push(lprocess)
end

function scheduler:Block(ms)
    local lprocess = self.current_lp
    if ms and ms > 0 then
        local nowtick = GetSysTick()
        lprocess.timeout = nowtick + ms
        if lprocess.index == 0 then
            self.m_timer:Insert(lprocess)
        else
            self.m_timer:Change(lprocess)
        end
    end
    lprocess.status = "block"
    coroutine.yield(lprocess.croutine)
    if lprocess.index ~= 0 then
        lprocess.timeout = 0		
        self.m_timer:Change(lprocess)
        self.m_timer:PopMin()
    end
end

function scheduler:Sleep(ms)
    local lprocess = self.current_lp
    if ms and ms > 0 then
        lprocess.timeout = C.GetSysTick() + ms
        if lprocess.index == 0 then
            self.m_timer:Insert(lprocess)
        else
            self.m_timer:Change(lprocess)
        end
        lprocess.status = "sleep"
    else
        lprocess.status = "yield"
    end
    coroutine.yield(lprocess.croutine)
end

function scheduler:Yield()
    self:Sleep(0)
end


function scheduler:Schedule()
	--local endtick = C.GetSysTick() + 5
    local size = self.runable:len()
	for i=1,size do
		local v = self.runable:pop()
		self.current_lp = v
		coroutine.resume(v.croutine,v)
		self.current_lp = nil
		if v.status == "yield" then
			self:Add2Active(v)
		elseif v.status == "dead" then
			print("a light process dead")
		end
		--if C.GetSysTick() > endtick then
		--	break
		--end		
	end
    local now = C.GetSysTick()
    while self.m_timer:Min() ~=0 and self.m_timer:Min() <= now do
        local lprocess = self.m_timer:PopMin()
        if lprocess.status == "block" or lprocess.status == "sleep" then
            self:Add2Active(lprocess)
        end
    end
    return self.runable:len()
end

function scheduler:WakeUp(lprocess)
    self:Add2Active(lprocess)
end

local global_sc = scheduler:new()
global_sc:init()

local function Yield()
    global_sc:Yield()
end

local function Sleep(ms)
    global_sc:Sleep(ms)
end

local function Block(ms)
    global_sc:Block(ms)
end

local function WakeUp(lprocess)
   global_sc:WakeUp(lprocess)
end

local function GetCurrentLightProcess()
    return global_sc.current_lp
end

local function GetLightProcessByIdentity(identity)
	return lightprocesses[identity]
end

local function Schedule()
	return global_sc:Schedule()
end

local function lp_start_fun(lp)
    --print("lp_start_fun")
	global_sc.CoroCount = global_sc.CoroCount + 1
	
    local _,err = pcall(lp.start_func,lp.ud)
    if err then
        print(err)
    end
    lightprocesses[lprocess.identity] = nil
	lp.status = "dead"
	lp.ud = nil
	global_sc.CoroCount = global_sc.CoroCount - 1
	--print("end lp_start_fun")
end

local function Spawn(func,ud)
    --print("node_spwan")
    local lprocess = LightProcess.NewLightProcess(lp_start_fun,ud,func)
    lightprocesses[lprocess.identity] = lprocess
    global_sc:Add2Active(lprocess)
end

return {
		Spawn = Spawn,
		Yield = Yield,
		Sleep = Sleep,
		Block = Block,
		WakeUp = WakeUp,
		GetCurrentLightProcess = GetCurrentLightProcess,
		GetLightProcessByIdentity = GetLightProcessByIdentity,
		Schedule = Schedule
}
