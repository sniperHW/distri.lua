local MinHeap = require "lua/minheap"
local Que =  require "lua/queue"


local sche = {
	ready_list = Que.New(),
	timer = MinHeap.New(),
	allcos = {},
	runningco = nil,
}

local stat_ready = 1
local stat_sleep = 2
local stat_yield = 3
local stat_dead  = 4
local stat_block = 5

local function add2Ready(co)
    if co.status == stat_ready then
        return
    end
    co.status = stat_ready
    sche.ready_list:Push(co)
end

local function Sleep(ms)
	local co = sche.runningco
	if ms and ms > 0 then
		co.timeout = GetSysTick() + ms
        if co.index == 0 then
            sche.timer:Insert(co)
        else
            sche.timer:Change(co)
        end
        co.status = stat_sleep		
	else
		co.status = stat_yield
	end
	coroutine.yield(co.coroutine)
end

local function Yield()
    Sleep(0)
end

local function Block(ms)
	local co = sche.runningco
    if ms and ms > 0 then
		ms = ms * 1000
        local nowtick = GetSysTick()
        co.timeout = nowtick + ms
        if co.index == 0 then
            sche.timer:Insert(co)
        else
            sche.timer:Change(co)
        end
    end
    co.status = stat_block
    coroutine.yield(co.coroutine)
    if co.index ~= 0 then
        co.timeout = 0		
        sche.timer:Change(co)
        sche.timer:PopMin()
    end
end

local function WakeUp(co)
    add2Ready(co)
end

local function Schedule(co)
	local readylist = sche.ready_list
	if co then
		local pre_co = sche.runningco
		sche.runningco = co
		coroutine.resume(co.coroutine,co)
		if co.status == stat_yield then
			add2Ready(co)
		end
		sche.runningco = pre_co
	else
		local yields = {}
		co = readylist:Pop()
		while co do
			sche.runningco = co
			coroutine.resume(co.coroutine,co)
			if co.status == stat_yield then
				table.insert(yields,co)
			--elseif co.status == stat_dead then
			end
			co = readylist:Pop()
		end
		local now = GetSysTick()
		local timer = sche.timer
		while timer:Min() ~=0 and timer:Min() <= now do
			co = timer:PopMin()
			if co.status == stat_block or co.status == stat_sleep then
				add2Ready(co)
			end
		end
		for k,v in pairs(yields) do
			add2Ready(v)
		end
    end
    return sche.ready_list:Len()
end

local function Running()
    return sche.runningco
end

local function GetCoByIdentity(identity)
	return sche.allcos[identity]
end

local function start_fun(co)
    local _,err = pcall(co.start_func,table.unpack(co.args))
    if err then
        print("error on start_fun:" .. err)
    end
    sche.allcos[co.identity] = nil
    co.status = stat_dead
end

local g_counter = 0
local function gen_identity()
	g_counter = g_counter + 1
	return "l" .. GetSysTick() .. "" .. g_counter
end

--产生一个coroutine在下次调用Schedule时执行
local function Spawn(func,...)
    local co = {index=0,timeout=0,identity=gen_identity(),start_func = func,args={...}}
    co.coroutine = coroutine.create(start_fun)
    if not sche.mainco then
		sche.mainco = co
    end   
    sche.allcos[co.identity] = co
    add2Ready(co)
end

--产生一个coroutine立刻执行
local function SpawnAndRun(func,...)
    local co = {index=0,timeout=0,identity=gen_identity(),start_func = func,args={...}}
    co.coroutine = coroutine.create(start_fun)
    if not sche.mainco then
		sche.mainco = co
    end   
    sche.allcos[co.identity] = co
    Schedule(co)
end

return {
		Spawn = Spawn,
		SpawnAndRun = SpawnAndRun,
		Yield = Yield,
		Sleep = Sleep,
		Block = Block,
		WakeUp = WakeUp,
		Running = Running,
		GetCoByIdentity = GetCoByIdentity,
		Schedule = Schedule
}
