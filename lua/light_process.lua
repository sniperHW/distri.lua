
local g_counter = 0

local light_process =
{
    index = 0,
    status = nil,
    croutine = nil,
    timeout = 0,
    ud = nil,
    identity = 0,
	start_func = nil
}

local function gen_identity()
	g_counter = g_counter + 1
	return "" .. C.GetSysTick() .. "" .. g_counter
end

function light_process:new(o)
  o = o or {}
  setmetatable(o, self)
  self.__index = self
  o.index = 0
  o.status = nil
  o.croutine = nil
  o.timeout = 0
  o.ud = nil
  o.start_func = nil
  o.identity = gen_identity()
  return o
end

local function NewLightProcess(routine,ud,st_func)
    local lprocess = light_process:new()
    lprocess.croutine = coroutine.create(routine)
    lprocess.ud = ud
    lprocess.start_func = st_func
    return lprocess
end

return {
  NewLightProcess = NewLightProcess
}

